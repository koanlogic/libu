/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <u/libu_conf.h>
#include <u/libu.h>
#include <toolbox/hmap.h>
#include <toolbox/pwd.h>

/* in-memory db */
static int u_pwd_db_new (u_pwd_t *pwd);
static int u_pwd_db_term (u_pwd_t *pwd);
static int u_pwd_db_load (u_pwd_t *pwd);
static int u_pwd_db_push (u_pwd_t *pwd, u_pwd_rec_t *rec);
static void __hmap_pwd_rec_free (void *val);  /* hook hmap free */

/* misc */
static int u_pwd_load (u_pwd_t *pwd);
static int u_pwd_need_reload (u_pwd_t *pwd);

/* u_pwd_rec_t */
static int u_pwd_rec_new (const char *user, const char *pass, 
        const char *opaque, u_pwd_rec_t **prec);
static int u_pwd_retr_mem (u_pwd_t *pwd, const char *user, 
        u_pwd_rec_t **prec);
static int u_pwd_retr_res (u_pwd_t *pwd, const char *user, 
        u_pwd_rec_t **prec);

/* res ops */
static int u_pwd_res_open (u_pwd_t *pwd);
static void u_pwd_res_close (u_pwd_t *pwd);

/* file specialization */
static int __file_open (const char *path, void **pfp);
static void __file_close (void *fp);
static char *__file_load (char *str, int size, void *fp);
static int __file_notify (const char *path, time_t last_update, 
        time_t *pnew_update);

/* a pwd instance context (will be passed along all u_pwd functions) */
struct u_pwd_s
{
    void *res_handler;  /* underlying storage resource: FILE*, io_t*, ... */
    char res_uri[U_FILENAME_MAX + 1];

    size_t hash_len;            /* hash'd password length */
    u_pwd_hash_cb_t cb_hash;    /* hash function for password hiding */

    u_pwd_open_cb_t cb_open;    /* function for opening the db */
    u_pwd_load_cb_t cb_load;    /* function for getting db records one by one */
    u_pwd_close_cb_t cb_close;  /* function for opening the db */

    u_pwd_notify_cb_t cb_notify;    /* function for notifying changes in the 
                                       master copy */

    time_t last_mod;    /* timestamp of master db's last update */
    int in_memory;      /* if set access is done via snapshot db */
    u_hmap_t *db;       /* in-memory master db snapshot */
};

/* each pwd line looks like this: "<user>:<password>[:opaque]\n":
 * the following holds the line tokenization result */
struct u_pwd_rec_s
{
    char *user;     /* credential owner */
    char *pass;     /* password (hashed or cleartext) */
    char *opaque;   /* optional application specific data (e.g. PSK hint) */
};

/**
    \defgroup pwd Password
    \{
        The \ref pwd module models the interface to a generic credential
        storage device which is password based.  An ::u_pwd_t object is the 
        handler through which all authentication and retrieval operations are 
        performed over a given password DB.  A password storage collects a 
        number of identical records which carry at least an username and 
        related password, plus an optional opaque blob which is intended to 
        transport application specific information.  Each of such records is 
        mediated by the ::u_pwd_rec_t object.  The storage is abstracted by 
        means of a brigade of callbacks that the programmer is supposed to 
        supply to the ::u_pwd_init function.  A specialization of such function
        is provided for file based password files as ::u_pwd_init_file.

        A simple authenticator based on the \ref pwd module follows:
        \code
    int main (int argc, char *argv[])
    {
        char c;
        int i, rc, in_memory = 0;
        u_pwd_t *pwd = NULL;
        char prompt[128];

        while ((c = getopt(argc, argv, "m")) != -1)
        {
            switch (c)
            {
                case 'm':
                    ++in_memory;
                    break;
                default:
                    con_err("usage: pwd [-m] user ...");
            }
        }

        argc -= optind;
        argv += optind;

        con_err_if (u_pwd_init_file("./passwd", NULL, 0, in_memory, &pwd));

        for (i = 0; i < argc; i++)
        {
            (void) u_snprintf(prompt, sizeof prompt, "%s: ", argv[i]);
            rc = u_pwd_auth_user(pwd, argv[i], getpass(prompt));
            u_con("auth %s", rc ? "failed" : "ok");
        }

        u_pwd_term(pwd);

        return EXIT_SUCCESS;
    err:
        return EXIT_FAILURE;
    }
        \endcode
    
        Where the passwd file structure can be as simple as:
        \code
    # "name":"password"[:"hint"]
    jake:joliet:blues
    steve:colonel:cropper
    donald:duck:dunn
        \endcode
       Note that the password field is in clear-text: in fact there was
       no hash callback supplied to the ::u_pwd_init_file function.
 */


/**
 *  \brief  Initialize a pwd instance
 *
 *  Get a new ::u_pwd_t object using the supplied attributes
 * 
 *  \param  res_uri     name of the master db resource
 *  \param  cb_open     method to open \p res_uri (get its handler)
 *  \param  cb_load     method to load \p res_uri lines one by one
 *  \param  cb_close    method to dispose \p res_uri handler (OPTIONAL)
 *  \param  cb_notify   method to notify changes in the master resource 
 *                      (OPTIONAL)
 *  \param  cb_hash     method to hash passwords (OPTIONAL)
 *  \param  hash_len    hashed password string length (needed if \p cb_hash
 *                      has been set)
 *  \param  in_memory   if true, keep an hash-map'd version of the master db
 *                      into memory (useful for huge and static db's)
 *  \param  ppwd        the pwd instance handler as a result value
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */ 
int u_pwd_init (const char *res_uri, u_pwd_open_cb_t cb_open, 
        u_pwd_load_cb_t cb_load, u_pwd_close_cb_t cb_close, 
        u_pwd_notify_cb_t cb_notify, u_pwd_hash_cb_t cb_hash, 
        size_t hash_len, int in_memory, u_pwd_t **ppwd)
{
    u_pwd_t *pwd;
   
    dbg_return_if (res_uri == NULL, ~0);
    dbg_return_if (cb_open == NULL, ~0);
    dbg_return_if (cb_load == NULL, ~0);
    /* cb_close is non-mandatory */
    /* cb_notify is non-mandatory */
    dbg_return_if (cb_hash && !hash_len, ~0);
    dbg_return_if (ppwd == NULL, ~0);

    /* make room for the instance context */
    pwd = u_zalloc(sizeof(u_pwd_t));
    dbg_err_sif (pwd == NULL);

    /* copy in supplied attributes and methods */
    pwd->res_handler = NULL;
    dbg_err_if (u_strlcpy(pwd->res_uri, res_uri, sizeof pwd->res_uri));

    pwd->hash_len = hash_len;
    pwd->cb_hash = cb_hash;
    pwd->cb_open = cb_open;
    pwd->cb_load = cb_load;
    pwd->cb_close = cb_close;
    pwd->cb_notify = cb_notify;
    pwd->last_mod = 0;
    pwd->in_memory = in_memory;
    pwd->db = NULL;

    /* NOTE: don't load to memory if requested (i.e. .in_memory != 0) here:
     * it will be done at very first u_pwd_retr() via u_pwd_need_reload() */

    *ppwd = pwd;

    return 0;
err:
    u_pwd_term(pwd);
    return ~0;
}

/**
 *  \brief  Retrieve a pwd record
 *
 *  Retrieve the ::u_pwd_rec_t object (if any) corresponding to the supplied 
 *  \p user from the ::u_pwd_t instance \p pwd
 * 
 *  \param  pwd     an already initialized pwd instance
 *  \param  user    user whose info shall be retrieved
 *  \param  prec    retrieved user record as a result argument (the record 
 *                  must be free'd using u_pwd_rec_free API).
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */ 
int u_pwd_retr (u_pwd_t *pwd, const char *user, u_pwd_rec_t **prec)
{
    dbg_return_if (pwd == NULL, ~0);
    dbg_return_if (user == NULL, ~0);
    dbg_return_if (prec == NULL, ~0);

    /* if in-memory snapshot is mantained, search there (in case on-storage
     * image has changed it will be resync'd automatically) */
    if (pwd->in_memory)
        return u_pwd_retr_mem(pwd, user, prec);

    return u_pwd_retr_res(pwd, user, prec);
}

/**
 *  \brief  Check if user has presented the right credential
 *
 *  Check if \p user has presented the right \p password to access the
 *  ::u_pwd_t object \p pwd
 *
 *  \param  pwd         an already initialized pwd instance
 *  \param  user        user whose credential has to be checked
 *  \param  password    the supplied credential
 *
 *  \retval  0  if authentication is ok
 *  \retval ~0  if authentication fails
 */ 
int u_pwd_auth_user (u_pwd_t *pwd, const char *user, const char *password)
{
    int rc;
    u_pwd_rec_t *rec = NULL;
    char *__p = NULL, __pstack[U_PWD_LINE_MAX];

    dbg_return_if (password == NULL, ~0);

    /* retrieve the pwd record */
    dbg_err_if (u_pwd_retr(pwd, user, &rec));

    /* hash if requested, otherwise do cleartext cmp */
    if (pwd->cb_hash)
    {
        /* create a buffer that fits the specific hash function */
        dbg_err_if ((__p = u_zalloc(pwd->hash_len)) == NULL);
        (void) pwd->cb_hash(password, strlen(password), __p);
    }
    else
    {
        (void) u_strlcpy(__pstack, password, sizeof __pstack);
        __p = __pstack;
    }

    rc = strcmp(__p, rec->pass);

    /* free __p if on heap */
    if (__p && (__p != __pstack))
        u_free(__p);

    /* rec ownership is ours only if hmap doesn't have it */
    if (!pwd->in_memory)
        u_pwd_rec_free(pwd, rec);

    return rc;
err:
    if (__p && (__p != __pstack))
        u_free(__p);

    if (!pwd->in_memory && rec)
        u_pwd_rec_free(pwd, rec);

    return ~0;
}

/**
 *  \brief  Dispose the supplied pwd instance
 *
 *  Dispose the supplied ::u_pwd_t object \p pwd
 * 
 *  \param  pwd     the pwd instance that shall be disposed
 *
 *  \return nothing
 */ 
void u_pwd_term (u_pwd_t *pwd)
{
    nop_return_if (pwd == NULL, );

    (void) u_pwd_db_term(pwd);

    U_FREE(pwd);

    return;
}

/**
 *  \brief  Init specialization for file-based password db
 *
 *  An ::u_pwd_init specialization for file-based password DBs
 * 
 *  \param  res_uri     name of the master db resource
 *  \param  cb_hash     method to hash passwords (OPTIONAL)
 *  \param  hash_len    hashed password string lenght (needed if \p cb_hash
 *                      has been set)
 *  \param  in_memory   if true, keep an hash-map'd version of the master db
 *                      into memory (useful for huge and static db's)
 *  \param  ppwd        the pwd instance handler as a result value
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */ 
int u_pwd_init_file (const char *res_uri, u_pwd_hash_cb_t cb_hash, 
        size_t hash_len, int in_memory, u_pwd_t **ppwd)
{
    return u_pwd_init (res_uri, __file_open, __file_load, __file_close, 
        __file_notify, cb_hash, hash_len, in_memory, ppwd);
}

/**
 *  \brief  Dispose a ::u_pwd_rec_t object 
 *
 *  Dispose the supplied ::u_pwd_rec_t object \p rec.  It MUST be called
 *  on ::u_pwd_rec_t records returned from ::u_pwd_retr when using "in memory" 
 *  pwd instances (for non "in memory" pwd's the function is a NOP)
 *
 *  \param  pwd     the pwd instance which owns \p rec
 *  \param  rec     the pwd_rec record to be disposed
 *
 *  \return nothing
 */ 
void u_pwd_rec_free (u_pwd_t *pwd, u_pwd_rec_t *rec)
{
    dbg_return_if (pwd == NULL, );
    dbg_return_if (rec == NULL, );

    U_FREE(rec->user);
    U_FREE(rec->pass);
    U_FREE(rec->opaque);

    u_free(rec);

    return;
}

/**
 *  \brief  Return the user field of the supplied pwd record
 *
 *  Get the user name attribute from the supplied ::u_pwd_rec_t object
 *
 *  \param  rec     an ::u_pwd_rec_t object returned by ::u_pwd_retr
 *
 *  \return a NUL-terminated string carrying the user name, 
 *          or \c NULL on error
 */ 
const char *u_pwd_rec_get_user (u_pwd_rec_t *rec)
{
    dbg_return_if (rec == NULL, NULL);
    return rec->user;
}

/**
 *  \brief  Return the password field of the supplied pwd record
 *
 *  Get the user password attribute from the supplied ::u_pwd_rec_t object
 *
 *  \param  rec     an ::u_pwd_rec_t object returned by ::u_pwd_retr
 *
 *  \return a NUL-terminated string carrying the user password, 
 *          or \c NULL on error
 */ 
const char *u_pwd_rec_get_password (u_pwd_rec_t *rec)
{
    dbg_return_if (rec == NULL, NULL);
    return rec->pass;
}

/**
 *  \brief  Return the opaque field of the supplied pwd record
 *
 *  Get the user opaque attribute from the supplied ::u_pwd_rec_t object
 *
 *  \param  rec     an ::u_pwd_rec_t object returned by ::u_pwd_retr
 *
 *  \return the opaque string (which can be \c NULL even if successful)
 */ 
const char *u_pwd_rec_get_opaque (u_pwd_rec_t *rec)
{
    dbg_return_if (rec == NULL, NULL);
    return rec->opaque;
}

/**
 *  \brief  Return the in_memory attribute from the supplied pwd instance
 *
 *  Get the \p in_memory attribute from the supplied ::u_pwd_t object, i.e.
 *  the fact that the password DB is memory cached or not.
 *
 *  \param  pwd    the ::u_pwd_t object to be inquired
 *
 *  \return return \c 0 in case it is not an in-memory pwd instance
 */ 
int u_pwd_in_memory (u_pwd_t *pwd)
{
    return pwd->in_memory;
}

/**
 *  \}
 */

static int u_pwd_load (u_pwd_t *pwd)
{
    dbg_return_if (pwd == NULL, ~0);
    dbg_return_if (!pwd->in_memory, ~0);

    /* wipe away old snapshot */
    if (pwd->db)
        (void) u_pwd_db_term(pwd);
    
    /* create new hash map and load master db contents into it */
    dbg_err_if (u_pwd_db_new(pwd));
    dbg_err_if (u_pwd_db_load(pwd));

    return 0;
err:
    return ~0;
}

static int u_pwd_retr_res (u_pwd_t *pwd, const char *user, 
        u_pwd_rec_t **prec)
{
    size_t lc, got_it = 0, ntoks;
    char ln[U_PWD_LINE_MAX], uu[U_PWD_LINE_MAX];
    char **toks = NULL;  /* expected line fmt is: "name:password[:opaque]\n" */
    u_pwd_rec_t *rec = NULL;

    dbg_return_if (pwd->res_uri == NULL, ~0);
    dbg_return_if (pwd->cb_load == NULL, ~0);
    /* cb_open consistency will be checked inside u_pwd_res_open */

    /* open master db */
    dbg_err_if (u_pwd_res_open(pwd));

    /* do suitable search string for strstr(3) */
    u_snprintf(uu, sizeof uu, "%s:", user);
    
    /* read line by line */
    for (lc = 1; pwd->cb_load(ln, sizeof ln, pwd->res_handler) != NULL; lc++)
    {
        /* skip comments */
        if (ln[0] == '#')
            continue;

        /* check if we're on user line, in case break the read loop... 
         * this is different from using the in-memory version in case an 
         * entry is duplicated: in-memory matches the last entry, here
         * we would get the first one */
        if (strstr(ln, uu) == ln)
        {
            got_it = 1;
            break;
        }
    }

    /* check if we've reached here due to simple loop exhaustion */
    dbg_err_ifm (!got_it, "user %s not found", user);

    /* remove terminating \n if needed */
    if (ln[strlen(ln) - 1] == '\n')
        ln[strlen(ln) - 1] = '\0';

    /* tokenize line: expect at least 2 fields: user and password */
    dbg_err_ifm (u_strtok(ln, ":", &toks, &ntoks) || ntoks < 2,
            "bad syntax at line %zu (%s)", lc, ln);

    /* create new record */
    dbg_err_if (u_pwd_rec_new(toks[0], toks[1], ntoks < 3 ? NULL : toks[2], 
                &rec));

    /* dispose u_strtok rubbish */
    u_strtok_cleanup(toks, ntoks);

    /* dispose resource handler (if a 'close' method has been set) */
    u_pwd_res_close(pwd);

    /* copy out */
    *prec = rec;

    return 0;
err:
    if (rec)
        u_pwd_rec_free(pwd, rec);

    if (toks)
        u_strtok_cleanup(toks, ntoks);

    u_pwd_res_close(pwd);

    return ~0;
}

static int u_pwd_res_open (u_pwd_t *pwd)
{
    dbg_return_if (pwd->cb_open == NULL, ~0);

    if (pwd->res_handler != NULL)
        u_warn("non-NULL resource handler will be lost");

    pwd->res_handler = NULL;

    return pwd->cb_open(pwd->res_uri, &pwd->res_handler);
}

static void u_pwd_res_close (u_pwd_t *pwd)
{
    nop_return_if (pwd->res_handler == NULL, );
    nop_return_if (pwd->cb_close == NULL, );

    pwd->cb_close(pwd->res_handler);
    pwd->res_handler = NULL;
    
    return;
}

static int u_pwd_rec_new (const char *user, const char *pass, 
        const char *opaque, u_pwd_rec_t **prec)
{
    u_pwd_rec_t *rec = NULL;

    dbg_return_if (user == NULL, ~0);
    dbg_return_if (pass == NULL, ~0);
    dbg_return_if (prec == NULL, ~0);

    rec = u_zalloc(sizeof(u_pwd_rec_t));
    dbg_err_sif (rec == NULL);

    rec->user = u_strdup(user);
    dbg_err_sif (rec->user == NULL);

    rec->pass = u_strdup(pass);
    dbg_err_sif (rec->pass == NULL);

    /* opaque field may be NULL */
    if (opaque)
    {
        rec->opaque = u_strdup(opaque);
        dbg_err_sif (rec->opaque == NULL);
    }

    *prec = rec;

    return 0;
err:
    if (rec)
    {
        U_FREE(rec->user);
        U_FREE(rec->pass);
        U_FREE(rec->opaque);
        u_free(rec);
    }
    return ~0;
}

static int u_pwd_retr_mem (u_pwd_t *pwd, const char *user, 
        u_pwd_rec_t **prec)
{
    u_pwd_rec_t *pr = NULL;

    dbg_return_if (pwd == NULL, ~0);
    dbg_return_if (user == NULL, ~0);
    dbg_return_if (prec == NULL, ~0);

    /* on error keep on working with the old in-memory db */
    dbg_ifb (u_pwd_need_reload(pwd))
        u_warn("error reloading master pwd file: using stale cache");

    dbg_err_if (pwd->db == NULL);

    pr = (u_pwd_rec_t *) u_hmap_easy_get(pwd->db, user);
    dbg_err_if (pr == NULL);

    *prec = pr;

    return 0;
err:
    return ~0;
}

static int u_pwd_need_reload (u_pwd_t *pwd)
{
    time_t update_timestamp;

    /* if needed parameters are not set return immediately (no error) */
    nop_return_if (!pwd->in_memory, 0);
    nop_return_if (pwd->cb_notify == NULL, 0);

    /* in case no update has been notified return */
    if (!pwd->cb_notify(pwd->res_uri, pwd->last_mod, &update_timestamp))
        return 0;

    /* update notified: set .last_mod */
    pwd->last_mod = update_timestamp;
    
    /* reload db to memory */
    return u_pwd_load(pwd);
}

static int u_pwd_db_new (u_pwd_t *pwd)
{
    u_hmap_opts_t *hopts = NULL;

    dbg_err_if (pwd == NULL);

    dbg_err_if (u_hmap_opts_new(&hopts));
    dbg_err_if (u_hmap_opts_set_val_freefunc(hopts, &__hmap_pwd_rec_free));
            
    return u_hmap_easy_new(hopts, &pwd->db);
err:
    if (hopts)
        u_hmap_opts_free(hopts);
    return ~0;
}

static int u_pwd_db_term (u_pwd_t *pwd)
{
    dbg_return_if (pwd == NULL, ~0);
        
    nop_return_if (pwd->db == NULL, 0);
            
    u_hmap_easy_free(pwd->db);
    pwd->db = NULL;

    return 0;
}

static int u_pwd_db_load (u_pwd_t *pwd)
{
    size_t lc, ntoks;
    char ln[U_PWD_LINE_MAX];
    char **toks = NULL;  /* line fmt is: "name:password[:hint]\n" */
    u_pwd_rec_t *rec = NULL;
    
    dbg_return_if (pwd->res_uri == NULL, ~0);
    dbg_return_if (pwd->cb_load == NULL, ~0);
    /* cb_open and cb_close will be checked inside u_pwd_res_{open,close} */

    /* open master db */
    dbg_err_if (u_pwd_res_open(pwd));

    for (lc = 1; pwd->cb_load(ln, sizeof ln, pwd->res_handler) != NULL; lc++)
    {
        /* cleanup strtok rubbish from the previous loop */
        if (toks)
        {
            u_strtok_cleanup(toks, ntoks);
            toks = NULL;
        }

        /* skip comment lines */
        if (ln[0] == '#')
            continue;

        /* remove trailing \n */
        if (ln[strlen(ln) - 1] == '\n')
            ln[strlen(ln) - 1] = '\0';

        /* tokenize line */
        dbg_ifb (u_strtok(ln, ":", &toks, &ntoks) || ntoks < 2)
        {
            u_info("bad syntax at line %zu (%s)", lc, ln);
            continue;
        }

        /* create u_pwd_rec_t from tokens */
        dbg_ifb (u_pwd_rec_new(toks[0], toks[1], ntoks < 3 ? NULL : toks[2], 
                    &rec))
        {
            u_info("could not create record for entry at line %zu", lc);
            continue;
        }

        /* push rec to db */
        dbg_ifb (u_pwd_db_push(pwd, rec))
        {
            u_info("could not push record for entry at line %zu", lc);
            u_pwd_rec_free(pwd, rec), rec = NULL;
        }

        rec = NULL;
    }

    if (toks)
        u_strtok_cleanup(toks, ntoks);
 
    if (rec)
        u_pwd_rec_free(pwd, rec);

    u_pwd_res_close(pwd);

    return 0;
err:
    if (rec)
        u_pwd_rec_free(pwd, rec);

    if (toks)
        u_strtok_cleanup(toks, ntoks);
 
    u_pwd_res_close(pwd);

    return ~0;
}

static int u_pwd_db_push (u_pwd_t *pwd, u_pwd_rec_t *rec)
{
    dbg_return_if (pwd->db == NULL, ~0);
    dbg_return_if (rec == NULL, ~0);
    dbg_return_if (rec->user == NULL, ~0);

    return u_hmap_easy_put(pwd->db, rec->user, (const void *) rec);
}

/* 
 * prefabricated callbacks
 */
static int __file_open (const char *path, void **pfp)
{
    FILE *fp = NULL;

    dbg_err_sif ((fp = fopen(path, "r")) == NULL);

    *pfp = (void *) fp;

    return 0;
err:
    return ~0;
}

static void __file_close (void *fp)
{
    dbg_return_sif (fclose((FILE *) fp), /* nothing */);
    return;
}

static char *__file_load (char *str, int size, void *fp)
{
    return fgets(str, size, (FILE *) fp);
}

/* in case file update has been detected, *pnew_update is also set */
static int __file_notify (const char *path, time_t last_update, 
        time_t *pnew_update)
{
    struct stat sb;

    dbg_err_if (path == NULL);

    dbg_err_sif (stat(path, &sb));

    if (sb.st_ctime != last_update)
    {
        *pnew_update = sb.st_ctime;
        return 1;
    }

    /* fall through (return false) */
err:
    return 0;
}

/* hmap glue */
static void __hmap_pwd_rec_free (void *val)
{
    u_pwd_t fake_pwd;

    fake_pwd.in_memory = 1;

    u_pwd_rec_free(&fake_pwd, (u_pwd_rec_t *) val);

    return;
}
