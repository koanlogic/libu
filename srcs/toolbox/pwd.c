/* 
 * Copyright (c) 2005-2008 by KoanLogic s.r.l. - All rights reserved.  
 */

static const char rcsid[] =
    "$Id: pwd.c,v 1.1 2008/03/10 15:32:52 tho Exp $";

#include <u/libu_conf.h>
#include <u/libu.h>
#include <toolbox/hmap.h>
#include <toolbox/pwd.h>

static int u_pwd_db_create (u_pwd_t *pwd);
static int u_pwd_db_destroy (u_pwd_t *pwd);
static int u_pwd_db_load (u_pwd_t *pwd);
static int u_pwd_db_push (u_pwd_t *pwd, u_pwd_rec_t *rec);
static int u_pwd_rec_new (const char *user, const char *pass, const char *hint,
        u_pwd_rec_t **prec);
static void u_pwd_rec_free (u_pwd_rec_t *rec);

struct u_pwd_s
{
    void *res_handler;      /* underlying storage resource: FILE*, io_t*, ... */
    u_hmap_t *db;           /* in-memory pwd db snapshot */
    hash_fn_t cb_hash;      /* hash function for password hiding */
    size_t hash_len;        /* hash'd password length */
    fgets_fn_t cb_fgets;    /* function for reading lines from the storage 
                             * resource */
};

/* each pwd line looks like this: "<user>:<password>[:hint]\n" */
struct u_pwd_rec_s
{
    char *user;     /* credential owner */
    char *pass;     /* password (hashed or cleartext) */
    char *hint;     /* optional (e.g. PSK hint) */
};

/**
 *  \defgroup pwd PWD
 *  \{
 */

/**
 *  \brief  Initialize the PWD module
 *
 *  \param  res         PWD file handler (\c FILE, \c io_t, etc.)
 *  \param  gf          fgets-like function used to retrieve line-by-line
 *                      the PWD records from the supplied resource
 *  \param  hf          password hash function to use
 *  \param  hash_len    hash function output buffer length
 *  \param  ppwd        the PWD instance handler as a result argument
 *
 *  \return \c 0 on success, \c ~0 on error
 */
int u_pwd_init (void *res, fgets_fn_t gf, hash_fn_t hf, size_t hash_len, 
        u_pwd_t **ppwd)
{
    u_pwd_t *pwd;
   
    dbg_return_if (res == NULL, ~0);
    dbg_return_if (gf == NULL, ~0);
    dbg_return_if (hf && !hash_len, ~0);
    dbg_return_if (ppwd == NULL, ~0);

    pwd = u_zalloc(sizeof(u_pwd_t));
    dbg_err_sif (pwd == NULL);

    pwd->cb_hash = hf;  /* may be NULL, if supplied hash_len must be > 0 */
    pwd->hash_len = hash_len;
    pwd->cb_fgets = gf;
    pwd->res_handler = res;

    dbg_err_if (u_pwd_load(pwd));

    *ppwd = pwd;

    return 0;
err:
    u_pwd_term(pwd);
    return ~0;
}

/**
 *  \brief  Load the PWD resource into memory
 *
 *  \param  pwd     PWD instance handler (must have been already initialized 
 *                  with u_pwd_init())
 *
 *  \return \c 0 on success, \c ~0 on error
 */
int u_pwd_load (u_pwd_t *pwd)
{
    dbg_return_if (pwd == NULL, ~0);
    dbg_return_if (pwd->res_handler == NULL, ~0);
    dbg_return_if (pwd->cb_fgets == NULL, ~0);

    if (pwd->db)
        (void) u_pwd_db_destroy(pwd);

    dbg_err_if (u_pwd_db_create(pwd));
    dbg_err_if (u_pwd_db_load(pwd));

    return 0;
err:
    return ~0;
}

/**
 *  \brief  Shutdown the PWD instance
 *
 *  \param  pwd     PWD module instance that has to be destroyed
 *
 *  \return \c 0 on success, \c ~0 on error
 */
int u_pwd_term (u_pwd_t *pwd)
{
    nop_return_if (pwd == NULL, 0);

    return u_pwd_db_destroy(pwd);
}

/**
 *  \brief  Retrieve a user PWD record
 *
 *  \param  pwd     PWD module instance
 *  \param  user    user to retrieve
 *  \param  prec    the retrieved record as a result argument
 *
 *  \return \c 0 on success, \c ~0 on error
 */
int u_pwd_retr (u_pwd_t *pwd, const char *user, u_pwd_rec_t **prec)
{
    u_hmap_o_t *hobj = NULL;

    dbg_return_if (pwd == NULL, ~0);
    dbg_return_if (pwd->db == NULL, ~0);
    dbg_return_if (prec == NULL, ~0);

    dbg_err_if (u_hmap_get(pwd->db, user, &hobj));
    *prec = (u_pwd_rec_t *) hobj->val;

    return 0;
err:
    return ~0;
}

/**
 *  \brief  Authorize the supplied user
 *
 *  \param  pwd     PWD module instance
 *  \param  user    user to authorize
 *  \param  pass    supplied password (cleartext or hashed)
 *
 *  \return \c 0 on success (user authorized), \c ~0 on error
 */
int u_pwd_auth_user (u_pwd_t *pwd, const char *user, const char *pass)
{
    int rc;
    u_pwd_rec_t *rec;
    char *__p = NULL, __pstack[U_PWD_LINE_MAX];

    /* retrieve the pwd record */
    dbg_err_if (u_pwd_retr(pwd, user, &rec));

    /* hash if requested, otherwise do cleartext cmp */
    if (pwd->cb_hash)
    {
        /* create a buffer that fits the specific hash function */
        dbg_err_if ((__p = u_zalloc(pwd->hash_len)) == NULL);
        (void) pwd->cb_hash(pass, strlen(pass), __p);
    }
    else
    {
        dbg_if (strlcpy(__pstack, pass, sizeof __pstack) >= sizeof __pstack);
        __p = __pstack;
    }

    rc = strcmp(__p, rec->pass);

    /* free __p if on heap */
    if (__p && (__p != __pstack))
        u_free(__p);

    return rc;
err:
    if (__p && (__p != __pstack))
        u_free(__p);
    return ~0;
}

/**
 *  \}
 */

static int u_pwd_db_create (u_pwd_t *pwd)
{
    dbg_return_if (pwd == NULL, ~0);

    return u_hmap_new(NULL, &pwd->db);
}

static int u_pwd_db_destroy (u_pwd_t *pwd)
{
    dbg_return_if (pwd == NULL, ~0);

    nop_return_if (pwd->db == NULL, 0);

    u_hmap_free(pwd->db);
    pwd->db = NULL;

    return 0;
}

static int u_pwd_db_load (u_pwd_t *pwd)
{
    size_t cnt;
    char ln[U_PWD_LINE_MAX];
    char *toks[3 + 1];  /* line fmt is: "name:password:hint\n" */
    u_pwd_rec_t *rec = NULL;

    dbg_return_if (pwd == NULL, ~0);
    dbg_return_if (pwd->res_handler == NULL, ~0);
    dbg_return_if (pwd->cb_fgets == NULL, ~0);

    for (cnt = 1; pwd->cb_fgets(ln, sizeof ln, pwd->res_handler) != NULL; cnt++)
    {
        /* skip comment lines */
        if (ln[0] == '#')
            continue;

        /* remove trailing \n */
        if (ln[strlen(ln) - 1] == '\n')
            ln[strlen(ln) - 1] = '\0';

        /* tokenize line */
        dbg_ifb (u_tokenize(ln, ":", toks, 3))
        {
            info("bad pwd format at line %zu (%s)", cnt, ln);
            continue;
        }

        /* create u_pwd_rec_t from tokens */
        dbg_ifb (u_pwd_rec_new(toks[0], toks[1], toks[2], &rec))
        {
            info("could not create record for entry at line %zu", cnt);
            continue;
        }

        /* push rec to db */
        dbg_ifb (u_pwd_db_push(pwd, rec))
            info("could not push record for entry at line %zu", cnt);
        rec = NULL;
    }

    if (rec)
        u_pwd_rec_free(rec);

    return 0;
}

static int u_pwd_db_push (u_pwd_t *pwd, u_pwd_rec_t *rec)
{
    char *hkey = NULL;
    u_hmap_o_t *hobj = NULL;

    dbg_return_if (pwd == NULL, ~0);
    dbg_return_if (pwd->db == NULL, ~0);
    dbg_return_if (rec == NULL, ~0);
    dbg_return_if (rec->user == NULL, ~0);

    hkey = u_strdup(rec->user);
    dbg_err_if (hkey == NULL);

    hobj = u_hmap_o_new((void *) hkey, (void *) rec);
    dbg_err_if (hobj == NULL);

    return u_hmap_put(pwd->db, hobj, NULL);
err:
    if (hkey)
        u_free(hkey);
    if (hobj)
        u_hmap_o_free(hobj);
    return ~0;
}

static int u_pwd_rec_new (const char *user, const char *pass, const char *hint,
        u_pwd_rec_t **prec)
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

    /* hint may be NULL */
    if (hint)
    {
        rec->hint = u_strdup(hint);    
        dbg_err_sif (rec->hint == NULL);
    }

    *prec = rec;

    return 0;
err:
    if (rec)
        u_pwd_rec_free(rec);
    return ~0;
}

static void u_pwd_rec_free (u_pwd_rec_t *rec)
{
    nop_return_if (rec == NULL, );

    U_FREE(rec->user);
    U_FREE(rec->pass);
    U_FREE(rec->hint);

    u_free(rec);

    return;
}
