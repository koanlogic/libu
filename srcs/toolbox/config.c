/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <fcntl.h>

#include <toolbox/carpal.h>
#include <toolbox/queue.h>
#include <toolbox/list.h>
#include <toolbox/config.h>
#include <toolbox/misc.h>
#include <toolbox/memory.h>
#include <toolbox/str.h>


TAILQ_HEAD(u_config_list_s, u_config_s);
typedef struct u_config_list_s u_config_list_t;

struct u_config_s
{
    TAILQ_ENTRY(u_config_s) np; /* next & prev pointers */
    char *key;                  /* config item key name */
    char *value;                /* config item value    */
    u_config_list_t children;   /* subkeys              */
    u_config_t *parent;         /* parent config obj    */
};

/* get() callback helper struct */
struct u_config_buf_s
{
    char *data;
    size_t len;
};

/* file system pre-defined driver */
extern u_config_driver_t u_config_drv_fs;

static u_config_t *u_config_get_root (u_config_t *c);
static int u_config_do_set_key (u_config_t *c, const char *key, const char *val,
        int overwrite, u_config_t **pchild);
static int cs_getline (u_config_gets_t cb, void *arg, u_string_t *ln);
static int u_config_do_load_drv (u_config_t *c, u_config_driver_t *drv, 
        void *arg, int overwrite);
static int u_config_include (u_config_t *c, u_config_driver_t *drv, 
        u_config_t *inckey, int overwrite);
static void u_config_del_key (u_config_t *c, u_config_t *child);
static int u_config_to_str (u_config_t *c, u_string_t *s);
static char *u_config_buf_gets (void *arg, char *buf, size_t size);
static int u_config_remove_comment(u_string_t *line);


/**
    \defgroup config Configuration
    \{
        In the context of the \ref config module, a config file consists of 
        sections and parameter assignements.
        
        A section begins with the name of the section - which can't contain
        whitespaces - is followed by a \c '{' (which must be on a line by 
        itself) and continues until the corresponding \c '}' is found.  
        
        Sections can contain other sections or parameter assignements of the 
        form:
    \code
    name    value
    \endcode

        The file is line-based, that is, each newline-terminated line 
        represents either a section name or a parameter assignement.  
        
        Comments start from the hash sign (\c '#') and eat all the chars 
        they encounter until the end of the line, unless the \c '#' is escaped -
        i.e. \c '\\#' - in which case it looses its special meaning and is
        treated as a normal name or value. 

        Section and parameter names are case sensitive.

        Leading and trailing whitespace in section and parameter names is 
        irrelevant.  Leading and trailing whitespace in a parameter 
        value is discarded.  Internal whitespace within a parameter value is 
        retained verbatim.

        As a corollary, any line containing only whitespace is ignored.

        The value in parameters is always a string (no quotes needed) whose 
        case is preserved.

        It is possible to use variable substitution which allows to define a 
        symbol name to be replaced by an arbitrary text string, with the 
        classic bottom-up scoping.
        
        Also, by means of the \c "include" directive it is possible to load 
        other configuration files into the one which is currently parsed.

        A simple example follows:
    \code
    last_name   blues

    type    musician

    # elwood's brother
    name
    {
        first   joliet
        middle  jake
        last    ${last_name}    # use substitution
    }

    address city of chicago
    \endcode

        A parameter name can be equivalently expressed through its "qualified 
        name" using the following dotted notation:
    \code
    # the very same elwood's brother
    name.first  joliet
    name.middle jake
    name.last   blues
    \endcode

        Once you have a well-formed configuration file, you can load it 
        into memory using ::u_config_load_from_file and then manipulate the 
        corresponding ::u_config_t object to obtain values of the parameters
        (\p keys) you are interested in via ::u_config_get_subkey_value and
        friends, as illustrated in the following example:
    \code
    u_config_t *top = NULL, *name_sect;
    const char *first_name, *middle_name, *last_name;

    // load and parse the configuration file "my.conf"
    dbg_err_if (u_config_load_from_file("./my.conf", &top));

    // get the section whose name is "name"
    dbg_err_if (u_config_get_subkey(top, "name", &name_sect));

    // get values of parameters "first", "middle" and "last" in section "name"
    first_name = u_config_get_subkey_value(name_sect, "first");
    middle_name = u_config_get_subkey_value(name_sect, "middle");
    last_name = u_config_get_subkey_value(name_sect, "last");
    ...
    u_config_free(top);
    \endcode

    After you have successfully loaded your configuration file, you could
    modify its values using ::u_config_set_value as needed, and than save
    it back to file with ::u_config_print_to_fp.  The loop is closed.

    When you're done, don't forget to release the top-level ::u_config_t 
    object via ::u_config_free.

    Other misc things that may be worth noting:
        - by means of the ::u_config_driver_t abstraction, any data source 
          is available for storing textual ::u_config_t objects: it is 
          sufficient to supply a set of custom callbacks interfacing the
          desired storage system;
        - an ::u_config_t object can be marshalled/unmarshalled via the 
          ::u_config_save_to_buf and ::u_config_load_from_buf functions
          respectively: as such it can be used as a generic message 
          framing facility (though not particularly space efficient).
 */

/**
 *  \brief  Sort config children 
 *
 *  Sort config children using the given comparison function. 
 *
 *  The comparison function must return an integer less than, equal to, or
 *  greater than zero if the first argument is respectively less than, 
 *  equal to, or greater than the second.
 *
 *  \param  c           an ::u_config_t object
 *  \param  config_cmp  a function which compares two ::u_config_t objects
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_config_sort_children (u_config_t *c, 
        int (*config_cmp)(u_config_t **, u_config_t **))
{
    u_config_t *child;
    int i, count;
    u_config_t **children = NULL;

    /* count children */
    count = 0;
    TAILQ_FOREACH(child, &c->children, np)
        count++;

    /* create an array of pointers */
    children = u_zalloc(count * sizeof(u_config_t *));
    dbg_err_if(children == NULL);

    /* populate the array of children */
    i = 0;
    TAILQ_FOREACH(child, &c->children, np)
        children[i++] = child;

    /* sort the list */
    qsort(children, count, sizeof(u_config_t *), 
            (int(*)(const void *, const void *)) config_cmp);

    /* remove all items from the list */
    while((child = TAILQ_FIRST(&c->children)) != NULL)
        TAILQ_REMOVE(&c->children, child, np);

    /* reinsert sorted items */
    for(i = 0; i < count; ++i)
        TAILQ_INSERT_TAIL(&c->children, children[i], np);

    U_FREE(children);

    return 0;
err:
    if(children)
        u_free(children);
    return ~0;
}

/**
 *  \brief  Print configuration to the given file stream pointer
 *
 *  Print a configuration object and its children to \c fp. For each config
 *  object all keys/values pair will be printed.
 *
 *  \param  c   an ::u_config_t object 
 *  \param  fp  reference to a file stream opened for writing
 *  \param  lev nesting level; must be zero
 *
 *  \return nothing
 */
void u_config_print_to_fp (u_config_t *c, FILE *fp, int lev)
{
#define U_CONFIG_INDENT(fp, l)  \
    do { int i; for(i = 1; i < l; ++i) fputs("\t", fp); } while (0);

    u_config_t *item;

    U_CONFIG_INDENT(fp, lev);

    /* "include" directives are skipped since that wouldn't give semantical 
     * equivalence to the dumped object (included key/val's are already 
     * there) */
    if (c->key && strcmp(c->key, "include"))
    {
        fprintf(fp, "%s", c->key);
        if (c->value != NULL) 
            fprintf(fp, "\t%s", c->value);
        fputs("\n", fp);
    }

    ++lev;

    if(u_config_has_children(c))
    {
        if(c->parent)
        {
            /* align braces to the previous level */
            U_CONFIG_INDENT(fp, lev - 1);
            fprintf(fp, "{\n");
        }

        TAILQ_FOREACH(item, &c->children, np)
            u_config_print_to_fp(item, fp, lev);

        if(c->parent)
        {
            U_CONFIG_INDENT(fp, lev - 1);
            fprintf(fp, "}\n");
        }
    }
#undef U_CONFIG_INDENT
}

/**
 *  \brief  Print configuration to \c stdout
 *
 *  Print a configuration object and its children to \c stdout. For each config
 *  object all keys/values pair will be printed.
 *
 *  \param  c   configuration object
 *  \param  lev nesting level; must be zero
 *
 *  \return nothing
 */
void u_config_print(u_config_t *c, int lev)
{
    u_config_print_to_fp(c, stdout, lev);
    return;
}

/**
 *  \brief  Add a child node with a given key to the supplied ::u_config_t 
 *          object
 *
 *  Create a child ::u_config_t object with key \p key, attached to the 
 *  supplied \p c.
 *
 *  \param  c   the parent ::u_config_t object
 *  \param  key the key which will be assigned to the newly created child
 *  \param  pc  the newly created child attached to \p pc
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_config_add_child (u_config_t *c, const char *key, u_config_t **pc)
{
    u_config_t *child = NULL;

    dbg_err_if(u_config_create(&child));

    child->parent = c;
    child->key = u_strdup(key);
    dbg_err_if(child->key == NULL);

    TAILQ_INSERT_TAIL(&c->children, child, np);

    *pc = child;

    return 0;
err:
    return ~0;
}

/**
 *  \brief  Get the n-th child by a given key
 *
 *  Get the child with key \p key at index \p n from \p c.
 *
 *  \param  c   the parent ::u_config_t object
 *  \param  key the key that must be searched
 *  \param  n   the index for the child object that we are expected to retrieve
 *
 *  \return the reference to the child if found, or \c NULL if no child was 
 *          found
 */
u_config_t *u_config_get_child_n (u_config_t *c, const char *key, int n)
{
    u_config_t *item;

    TAILQ_FOREACH(item, &c->children, np)
    {
        if((key == NULL || strcmp(item->key, key) == 0) && n-- == 0)
            return item;  /* found */
    }

    return NULL; /* not found */
}

/**
 *  \brief  Get a child with a given key
 *
 *  Get the child with key \p key from its parent ::u_config_t object \p c
 *
 *  \param  c   the parent ::u_config_t object
 *  \param  key the key that shall be searched
 *
 *  \return the reference to the found child, or \c NULL if no child was found
 */
u_config_t *u_config_get_child (u_config_t *c, const char *key)
{
    return u_config_get_child_n(c, key, 0);
}

/**
 *  \brief  Get the n-th configuration object child corresponding to a given 
 *          subkey
 *
 *  Get the child config object corresponding to the given subkey \p subkey 
 *  at index \p n
 *
 *  \param  c       the parent ::u_config_t object
 *  \param  subkey  a subkey value
 *  \param  n       the index at which the child object is retrieved
 *  \param  pc      a result argument carrying the found object reference
 *                  if the search was successful
 *
 *  \retval  0  on success
 *  \retval ~0  on error (i.e. no match)
 */
int u_config_get_subkey_nth (u_config_t *c, const char *subkey, int n, 
        u_config_t **pc)
{
    u_config_t *child = NULL;
    char *first_key = NULL, *p;

    if ((p = strchr(subkey, '.')) == NULL)
    {
        if ((child = u_config_get_child_n(c, subkey, n)) != NULL)
        {
            *pc = child;
            return 0;
        } 
    } 
    else 
    {
        if ((first_key = u_strndup(subkey, p - subkey)) != NULL)
        {
            child = u_config_get_child(c, first_key);
            U_FREE(first_key);
        }

        if (child != NULL)
            return u_config_get_subkey(child, ++p, pc);
    }

    return ~0; /* not found */
}

/**
 *  \brief  Get a child configuration object corresponding to a given subkey
 *
 *  Get a child config object (actually the first one) corresponding to the 
 *  given subkey \p subkey from parent \p c
 *
 *  \param  c       the parent ::u_config_t object
 *  \param  subkey  a subkey value
 *  \param  pc      the found child (if found) as a result argument
 *
 *  \retval  0  on success
 *  \retval ~0  on error (i.e. no match)
 */
int u_config_get_subkey (u_config_t *c, const char *subkey, u_config_t **pc)
{
    return u_config_get_subkey_nth(c, subkey, 0, pc);
}

/**
 *  \brief  Assign \p val to \p c's key
 *
 *  Assign the given value \p val to the ::u_config_t object \p c
 *
 *  \param  c   the ::u_config_t object that will be manipulated
 *  \param  val the value that will be assigned to \p c
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */
int u_config_set_value (u_config_t *c, const char *val)
{
    u_config_t *root, *ignore;
    const char *varval, *vs, *ve, *p;
    u_string_t *var = NULL, *value = NULL;

    dbg_err_if(c == NULL);

    /* free() previous value if any */
    if(c->value)
    {
        U_FREE(c->value);
        c->value = NULL;
    } 

    if(val)
    {
        dbg_err_if(u_string_create(NULL, 0, &var));
        dbg_err_if(u_string_create(NULL, 0, &value));

        root = u_config_get_root(c);
        dbg_err_if(root == NULL);

        /* search and replace ${variables} */
        vs = ve = val;
        for(; vs && *vs && (p = strstr(vs, "${")) != NULL; vs = ++ve)
        {   /* variable substituion */
            dbg_err_if(u_string_append(value, vs, p-vs));

            /* skip ${ */
            vs = p+2;               

            /* look for closing bracket */
            ve = strchr(vs, '}');
            dbg_err_if(ve == NULL); /* closing bracket missing */

            /* get the variable name/path */
            dbg_err_if(u_string_set(var, vs, ve-vs));

            /* first see if the variable can be resolved in the local scope
               otherwise resolve it from the root */
            root = c->parent;
            if(u_config_get_subkey(root, u_string_c(var), &ignore))
                root = u_config_get_root(c);
            dbg_err_if(root == NULL);

            /* append the variable value */
            varval = u_config_get_subkey_value(root, u_string_c(var));
            if(varval != NULL)
                dbg_err_if(u_string_append(value, varval, strlen(varval)));
        }
        if(ve && *ve)
            dbg_err_if(u_string_append(value, ve, strlen(ve)));

        u_string_trim(value); /* remove leading and trailing spaces */

        c->value = u_strdup(u_string_c(value));;
        dbg_err_if(c->value == NULL);

        u_string_free(value);
        u_string_free(var);
    }

    return 0;
err:
    if(value)
        u_string_free(value);
    if(var)
        u_string_free(var);
    return ~0;
}

/**
 *  \brief  Append the given key/value pair into a config object
 *
 *  Append the given \p key, \p val pair into the supplied ::u_config_t object
 *  \p c.
 *
 *  \param  c   the ::u_config_t object that is manipulated
 *  \param  key the key to insert
 *  \param  val the value to insert
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */
int u_config_add_key (u_config_t *c, const char *key, const char *val)
{
    return u_config_do_set_key(c, key, val, 0 /* don't overwrite */, NULL);
}

/**
 *  \brief  Put the given key/value pair into a config object
 *
 *  Put the given \p key, \p val pair into the supplied ::u_config_t object
 *  \p c.  If the same key is found into \p c, it is overwritten.
 *
 *  \param  c   the ::u_config_t object that is manipulated
 *  \param  key the key to insert
 *  \param  val the value to insert
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */
int u_config_set_key (u_config_t *c, const char *key, const char *val)
{
    return u_config_do_set_key(c, key, val, 1 /* overwrite */, NULL);
}

/**
 *  \brief  Load from an opaque data source
 *
 *  Load configuration data from a data source; a gets()-like function must
 *  be provided by the caller to read the configuration line-by-line.
 *
 *  \param  c           an ::u_config_t object
 *  \param  cb          line reader callback with \c gets(3) prototype
 *  \param  arg         opaque argument passed to the \p cb function
 *  \param  overwrite   if set to \c 1 overwrite keys with the same name,
 *                      otherwise new key with the same name will be added
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */
int u_config_load_from (u_config_t *c, u_config_gets_t cb, void *arg, 
        int overwrite)
{
    u_config_driver_t drv = { NULL, NULL, cb, NULL };

    dbg_err_if(u_config_do_load_drv(c, &drv, arg, overwrite));

    return 0;
err:
    return ~0;
}

/**
 *  \brief  Load a configuration file.
 *
 *  Fill a config object with key/value pairs loaded from the configuration
 *  file linked to the file descriptor \p fd. If \c overwrite is not zero,
 *  values of keys with the same name will overwrite previous values, otherwise
 *  new keys with the same name will be added.
 *
 *  \param  c           a ::u_config_t object
 *  \param  fd          file descriptor opened for reading
 *  \param  overwrite   if set to \c 1 overwrite keys with the same name,
 *                      otherwise new key with the same name will be added
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */
int u_config_load (u_config_t *c, int fd, int overwrite)
{
    FILE *file = NULL;

    /* must dup because fclose() calls close(2) on fd */
    file = fdopen(dup(fd), "r");
    dbg_err_if(file == NULL);

    dbg_err_if(u_config_do_load_drv(c, &u_config_drv_fs, file, overwrite));

    fclose(file);

    return 0;
err:
    U_FCLOSE(file);
    return ~0;
}

/**
 *  \brief  Create a configuration object read from a configuration file
 *
 *  Create a configuration object reading from a config file
 *
 *  \param  file      the configuration file path name
 *  \param  pc        value-result that will get the new configuration object
 *
 *  \retval  0  on success
 *  \retval ~0  on error
 */
int u_config_load_from_file (const char *file, u_config_t **pc)
{
    dbg_return_if (file == NULL, ~0);
    dbg_return_if (pc == NULL, ~0);

    dbg_err_if (u_config_load_from_drv(file, &u_config_drv_fs, 0, pc));

    return 0;
err:
    return ~0;
}

/**
 *  \brief  Create a config object
 *
 *  Create a new ::u_config_t object. The returned object is completely empty,
 *  use ::u_config_set_key to set its key/value pairs
 *
 *  \param  pc  value-result that will get the new configuration object
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_config_create (u_config_t **pc)
{
    u_config_t *c = NULL;

    c = u_zalloc(sizeof(u_config_t));
    dbg_err_sif (c == NULL);

    TAILQ_INIT(&c->children);

    *pc = c;

    return 0;
err:
    if(c)
        u_config_free(c);
    return ~0;
}

/**
 *  \brief  Remove a child (and all its sub-children) from a config object
 *
 *  Remove a child and all its sub-children from a config object.
 *
 *  \param  c       parent ::u_config_t object
 *  \param  child   child to remove
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_config_del_child (u_config_t *c, u_config_t *child)
{
    u_config_t *item;

    TAILQ_FOREACH(item, &c->children, np)
    {
        if(item == child)   /* found */
        {   
            u_config_del_key(c, child);
            dbg_err_if(u_config_free(child));
            return 0;
        }
    }

err:
    return ~0;
}

/**
 *  \brief  Free a config object and all its children.
 *
 *  Free a config object and all its children.
 *
 *  \param  c   configuration object
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_config_free (u_config_t *c)
{
    u_config_t *child = NULL;

    if (c)
    {
        /* free all children */
        while ((child = TAILQ_FIRST(&c->children)) != NULL)
        {
            u_config_del_key(c, child);
            (void) u_config_free(child);
        }

        /* free parent */
        U_FREE(c->key);
        U_FREE(c->value);
        u_free(c);
    }

    return 0;
}

/**
 *  \brief  Return the key of a config object.
 *
 *  Return the key string of the supplied ::u_config_t object \p c
 *
 *  \param  c   an ::u_config_t object
 *
 *  \return the key string reference on success (which could be \c NULL), 
 *          and \c NULL if \p c is \c NULL
 */
const char *u_config_get_key (u_config_t *c)
{
    dbg_return_if (c == NULL, NULL);

    return c->key;
}

/**
 *  \brief  Return the value of a config object.
 *
 *  Return the value string of a config object.
 *
 *  \param  c   configuration object
 *
 *  \return the value string on success (which could be \c NULL), and \c NULL 
 *          if \p c is \c NULL
 */
const char *u_config_get_value (u_config_t *c)
{
    dbg_return_if (c == NULL, NULL);

    return c->value;
}

/**
 *  \brief  Return the value of a subkey.
 *
 *  Return the value of the child config object whose key is \p subkey.
 *
 *  \param  c       configuration object
 *  \param  subkey  the key value of the child config object
 *
 *  \return the value string on success, \c NULL on failure
 */
const char *u_config_get_subkey_value (u_config_t *c, const char *subkey)
{
    u_config_t *skey;

    nop_err_if(u_config_get_subkey(c, subkey, &skey));

    return u_config_get_value(skey);
err:
    return NULL;
}

/**
 *  \brief  Return the value of an integer subkey.
 *
 *  Return the integer value (::u_atoi is used for conversion) of the child 
 *  config object whose key is \p subkey.
 *
 *  \param  c       configuration object
 *  \param  subkey  the key value of the child config object
 *  \param  def     the value to return if the key is missing
 *  \param  out     on exit will get the integer value of the key
 *
 *  \retval  0  on success
 *  \retval ~0  if the key value is not an int
 */
int u_config_get_subkey_value_i (u_config_t *c, const char *subkey, int def, 
        int *out)
{
    const char *v;

    if((v = u_config_get_subkey_value(c, subkey)) == NULL)
    {
        *out = def;
        return 0;
    }

    dbg_err_if (u_atoi(v, out));

    return 0;
err:
    return ~0;
}

/**
 *  \brief  Return the value of a boolean subkey.
 *
 *  Return the bool value of the child config object whose key is \p subkey.
 *  Recognized bool values are (case insensitive) \c yes/no, \c 1/0, 
 *  \c enable/disable.
 *
 *  \param c         configuration object
 *  \param subkey    the key value of the child config object
 *  \param def       the value to return if the key is missing 
 *  \param out       on exit will get the bool value of the key
 *
 *  \retval  0  on success
 *  \retval ~0  if the key value is not a bool keyword
 */
int u_config_get_subkey_value_b (u_config_t *c, const char *subkey, int def, 
        int *out)
{
    const char *true_words[]  = { "yes", "enable", "1", "on", NULL };
    const char *false_words[] = { "no", "disable", "0", "off", NULL };
    const char *v, *w;
    int i;

    if((v = u_config_get_subkey_value(c, subkey)) == NULL)
    {
        *out = def;
        return 0;
    }

    for(i = 0; (w = true_words[i]) != NULL; ++i)
    {
        if(!strcasecmp(v, w))
        {
            *out = 1;
            return 0; /* ok */
        }
    }

    for(i = 0; (w = false_words[i]) != NULL; ++i)
    {
        if(!strcasecmp(v, w))
        {
            *out = 0;
            return 0; /* ok */
        }
    }

    return ~0; /* not-bool value */
}

/**
 *  \brief  Load a configuration object using the supplied ::u_config_driver_t
 *
 *  Load a configuration object from the resource specified by \p uri, using 
 *  the supplied ::u_config_driver_t \p drv.  If successful, the loaded object
 *  will be returned as a result-argument at \p *pc.
 *
 *  \param  uri         a string indicating the name of the resource from which
 *                      the config object will be loaded.  If set (i.e. non
 *                      \c NULL), it is supplied as-is to the open callback of 
 *                      the \p drv object 
 *  \param  drv         the configured ::u_config_driver_t that implements the
 *                      interface to the resorce from which the config object
 *                      is loaded
 *  \param  overwrite   if set to \c 1 keys with the same name will be 
 *                      overwritten, otherwise a key with the same name of
 *                      an already existing one will be appended
 *  \param  pc          on success, contains the reference to the created
 *                      ::u_config_t object
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_config_load_from_drv (const char *uri, u_config_driver_t *drv, 
        int overwrite, u_config_t **pc)
{
    u_config_t *c = NULL;
    void *arg = NULL;

    dbg_err_if (uri == NULL);
    dbg_err_if (pc == NULL);
    dbg_err_if (drv == NULL);

    dbg_err_if (drv->gets == NULL);

    dbg_err_if (u_config_create(&c));

    if(drv->open)
        dbg_err_if (drv->open(uri, &arg));

    dbg_err_if (u_config_do_load_drv(c, drv, arg, overwrite));

    if(drv->close)
        dbg_err_if (drv->close(arg));

    *pc = c;

    return 0;
err:
    if(arg && drv->close)
        drv->close(arg);
    if(c)
        u_config_free(c);
    return ~0;
}

/**
 *  \brief  Tell if a given config object has children nodes
 *
 *  Tell if the supplied ::u_config_t object \p c has at least one child node
 *
 *  \param  c   an ::u_config_t object
 *
 *  \retval  1  if answer is yes
 *  \retval  0  if answer is no
 */
int u_config_has_children (u_config_t *c)
{
    return (TAILQ_FIRST(&c->children) != NULL);
}

/**
 *  \brief  Marshall a config object into a memory buffer
 *
 *  Serialize the supplied ::u_config_t object \p c and push it into the 
 *  \p size bytes memory buffer \p buf.  In case the supplied buffer is too 
 *  small the function returns an error.
 *
 *  \param  c       the candidate ::u_config_t object for serialization
 *  \param  buf     a pre-allocated buffer of \p size bytes
 *  \param  size    size in bytes of \p buf
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_config_save_to_buf (u_config_t *c, char *buf, size_t size)
{
    u_string_t *s = NULL;

    dbg_err_if(u_string_create(NULL, 0, &s));

    dbg_err_if(u_config_to_str(c, s));

    /* buffer too small */
    warn_err_if(u_string_len(s) >= size);

    (void) u_strlcpy(buf, u_string_c(s), size);

    u_string_free(s);

    return 0;
err:
    if(s)
        u_string_free(s);
    return ~0;
}

/**
 *  \brief  Unmarshall a config object from a memory buffer
 *
 *  Read a config object from the memory buffer \p buf.  In case of success,
 *  the newly created ::u_config_t object is given back via the result-argument
 *  \p pc.
 *
 *  \param  buf a memory buffer containing a serialized config object
 *  \param  len the dimension of \p buf in bytes
 *  \param  pc  a result-argument carrying the deserialized config object
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
int u_config_load_from_buf (char *buf, size_t len, u_config_t **pc)
{
    u_config_driver_t drv = { NULL, NULL, u_config_buf_gets, NULL };
    struct u_config_buf_s arg = { buf, len };
    u_config_t *c = NULL;

    dbg_err_if(u_config_create(&c));

    dbg_err_if(u_config_do_load_drv(c, &drv, &arg, 0));

    *pc = c;

    return 0;
err:
    if(c)
        u_config_free(c);
    return ~0;
}

/**
 *  \brief  Traverse the configuration tree.
 *          
 *  Traverse the configuration tree rooted at \p c and invoke the supplied
 *  callback function \p cb on each visited node.  The walk strategy \p s can 
 *  be one of ::U_CONFIG_WALK_PREORDER or ::U_CONFIG_WALK_POSTORDER for pre- 
 *  or post-order respectively.
 *
 *  \param  c   A configuration tree.
 *  \param  s   Walk strategy, one of ::u_config_walk_t values.
 *  \param  cb  Callback function to be invoked on each visited node.
 *
 *  \return nothing.
 */
void u_config_walk (u_config_t *c, u_config_walk_t s, void (*cb)(u_config_t *))
{
   int i;
   u_config_t *cc;

   for (i = 0; (cc = u_config_get_child_n(c, NULL, i)) != NULL; i++)
   {
       if (s == U_CONFIG_WALK_PREORDER)
           cb(cc);

       if (u_config_has_children(cc))
           u_config_walk(cc, s, cb);

       if (s == U_CONFIG_WALK_POSTORDER)
           cb(cc);
   }
}

/**
 *      \}
 */

static u_config_t *u_config_get_root (u_config_t *c)
{
    while(c->parent)
        c = c->parent;
    return c;
}

static int u_config_do_set_key(u_config_t *c, const char *key, const char *val,
        int overwrite, u_config_t **pchild)
{
    u_config_t *child = NULL;
    char *p, *child_key;

    if((p = strchr(key, '.')) == NULL)
    {
        child = u_config_get_child(c, key);
        if(child == NULL || !overwrite)
        {   /* there's no such child, add a new child */
            dbg_err_if(u_config_add_child(c, key, &child));
        } 
        dbg_err_if(u_config_set_value(child, val));

        /* return the child we just added/modified */
        if(pchild)
            *pchild = child;
    } else {
        child_key = u_strndup(key, p-key);
        dbg_err_if(child_key == NULL);
        if((child = u_config_get_child(c, child_key)) == NULL)
            dbg_err_if(u_config_add_child(c, child_key, &child));
        U_FREE(child_key);
        return u_config_do_set_key(child, ++p, val, overwrite, NULL);
    }
    return 0;
err:
    return ~0;
}

static int cs_getline (u_config_gets_t cb, void *arg, u_string_t *ln)
{
    enum { BUFSZ = 1024 };
    char buf[BUFSZ], *p = NULL;
    ssize_t len;

    u_string_clear(ln);

    while((p = cb(arg, buf, BUFSZ)) != NULL)
    {
        len = strlen(buf);
        if(len == 0)
            break; /* empty line */
        dbg_err_if(u_string_append(ln, buf, --len));
        if(!u_isnl(buf[len]))
            continue; /* line's longer the bufsz (or eof);get next line chunk */
        else
            break;
    }

    nop_err_if(p == NULL); /* eof */

    return 0;
err:
    return ~0;
}

static int u_config_do_load_drv (u_config_t *c, u_config_driver_t *drv, 
        void *arg, int overwrite)
{
    enum { MAX_NEST_LEV = 20 };
    u_string_t *line = NULL, *key = NULL, *lastkey = NULL, *value = NULL;
    const char *ln, *p;
    size_t len;
    int lineno = 1;
    u_config_t *child = NULL;
    u_config_t *subkey;

    dbg_err_if(u_string_create(NULL, 0, &line));
    dbg_err_if(u_string_create(NULL, 0, &key));
    dbg_err_if(u_string_create(NULL, 0, &value));
    dbg_err_if(u_string_create(NULL, 0, &lastkey));

    dbg_err_if(drv->gets == NULL);

    for(; cs_getline(drv->gets, arg, line) == 0; u_string_clear(line), ++lineno)
    {
        dbg_err_if (u_config_remove_comment(line));

        /* remove leading and trailing blanks */
        dbg_err_if(u_string_trim(line));

        ln = u_string_c(line);
        len = u_string_len(line);

        /* remove trailing nl(s) */
        while(len && u_isnl(ln[len-1]))
            u_string_set_length(line, --len);

        if(len == 0)
            continue; /* empty line */

        /* eat leading blanks */
        for(; u_isblank(*ln); ++ln);

        if(ln[0] == '{')
        {   /* group config values */
            if(u_string_len(lastkey) == 0)
                crit_err("config error [line %d]: { not after a no-value key", 
                         lineno);
            if(!u_isblank_str(++ln))
                crit_err("config error [line %d]: { or } must be the "
                         "only not-blank char in a line", lineno);

            /* modify the existing child (when overwriting) or add a new one */
            if(!overwrite || 
               (child = u_config_get_child(c, u_string_c(lastkey))) == NULL)
            {
                dbg_err_if(u_config_add_child(c, u_string_c(lastkey), &child));
            }

            dbg_err_if(u_config_do_load_drv(child, drv, arg, overwrite));

            dbg_err_if(u_string_clear(lastkey));

            continue;
        } else if(ln[0] == '}') {
            crit_err_ifm(c->parent == NULL,"config error: unmatched '}'");
            if(!u_isblank_str(++ln))
                dbg_err("config error [line %d]: { or } must be the "
                         "only not-blank char in a line", lineno);
            break; /* exit */
        }

        /* find the end of the key string */
        for(p = ln; *p && !u_isblank(*p); ++p);

        /* set the key */
        dbg_err_if(u_string_set(key, ln, p-ln));

        /* set the value */
        dbg_err_if(u_string_set(value, p, strlen(p)));
        dbg_err_if(u_string_trim(value));

        if(!strcmp(u_string_c(key), "include") ||     /* forced dependency */
                !strcmp(u_string_c(key), "-include")) /* optional dependency */
        {
            crit_err_ifm(u_string_len(value) == 0, "missing include filename");

            /* add to the include key to the list to resolve ${} vars */
            dbg_err_if(u_config_do_set_key(c, u_string_c(key), 
                        u_string_c(value), 0, &subkey));

            /* load the included file */
            if (u_string_c(key)[0] == '-')  /* failure is not critical */
                dbg_if(u_config_include(c, drv, subkey, overwrite));
            else                            /* failure is critical */
                dbg_err_if(u_config_include(c, drv, subkey, overwrite));
        } 

        /* if the valus is empty an open bracket will follow, save the key */
        if(u_string_len(value) == 0)
        {
            dbg_err_if(u_string_set(lastkey, ln, p-ln));
            continue;
        }

        /* add to the var list */
        dbg_err_if(u_config_do_set_key(c, 
                        u_string_c(key), 
                        u_string_len(value) ? u_string_c(value) : NULL, 
                        overwrite, NULL));
    }
    
    u_string_free(lastkey);
    u_string_free(value);
    u_string_free(key);
    u_string_free(line);

    return 0;
err:
    if(lastkey)
        u_string_free(lastkey);
    if(key)
        u_string_free(key);
    if(value)
        u_string_free(value);
    if(line)
        u_string_free(line);
    return ~0;
}

static int u_config_include (u_config_t *c, u_config_driver_t *drv, 
        u_config_t *inckey, int overwrite)
{
    u_config_t *subkey;
    int i;
    const char *p, *path;
    void *arg = NULL;
    char uri[U_FILENAME_MAX + 1];

    dbg_err_if(c == NULL);

    path = u_config_get_value(inckey);
    dbg_err_if(path == NULL);

    for(i = 0; u_config_get_subkey_nth(c, "include", i, &subkey) == 0; ++i)
    {
        if(subkey != inckey && !strcmp(path, u_config_get_value(subkey)))
            crit_err("circular dependency error loading %s", path);
    }

    /* find the end of the key string */
    for(p = path; *p && u_isblank(*p); ++p);

    dbg_err_if(p == NULL);

    crit_err_ifm (drv->open == NULL,
        "'include' feature used but the 'open' driver callback is not defined");

    /* resolv the include filename */
    if(drv->resolv)
    {
        dbg_err_if (drv->resolv(p, uri, sizeof(uri)));

        p = uri;
    } 

    crit_err_ifm (drv->open(p, &arg),
        "unable to access input file: %s", p);

    dbg_err_if (u_config_do_load_drv(c, drv, arg, overwrite));

    if(drv->close)
        crit_err_ifm (drv->close(arg),
            "unable to close input file: %s", p);
    else
        u_warn("the 'close' driver callback is not defined, not closing...");

    return 0;
err:
    if(arg && drv->close)
        drv->close(arg);
    return ~0;
}

/*  Delete a child config object. 'child' must be a child of 'c' */
static void u_config_del_key (u_config_t *c, u_config_t *child)
{
    TAILQ_REMOVE(&c->children, child, np);
}

static int u_config_to_str (u_config_t *c, u_string_t *s)
{
    u_config_t *item;

    dbg_err_if(c == NULL);
    dbg_err_if(s == NULL);

    if(c->key && strcmp(c->key, "include"))
        u_string_aprintf(s, "%s %s\n", c->key, (c->value ? c->value : ""));
    
    if(u_config_has_children(c))
    {
        if(c->parent)
            u_string_aprintf(s, "%s", "{\n");

        TAILQ_FOREACH(item, &c->children, np)
            u_config_to_str(item, s);

        if(c->parent)
            u_string_aprintf(s, "%s", "}\n");
    }

    return 0;
err:
    return ~0;
}

static char *u_config_buf_gets (void *arg, char *buf, size_t size)
{
    struct u_config_buf_s *g = (struct u_config_buf_s*)arg;
    char c, *s = buf;

    dbg_err_if(arg == NULL);
    dbg_err_if(buf == NULL);
    dbg_err_if(size == 0);

    if(g->len == 0 || g->data[0] == '\0')
        return NULL;

    size--; /* save a char for the trailing \0 */

    for(; size && g->len; --size)
    {
        c = *g->data;

        g->data++, g->len--;

        *s++ = c;

        if(c == '\n')
            break;
    }
    *s = '\0';

    /* dbg("ln: [%s]", buf); */

    return buf;
err:
    return NULL;
}

static int u_config_remove_comment(u_string_t *line)
{
    char last, ln[2048], *p;

    dbg_return_if (line == NULL, ~0);

    dbg_err_if (u_strlcpy(ln, u_string_c(line), sizeof ln));

    for (last = '\0', p = ln; *p != '\0'; )
    {
        if (*p == '#')
        {
            /* See if '#' was escaped. */
            if (last != '\\')
            {
                *p = '\0';
                break;
            }
            else
            {
                size_t rlen = strlen(p);

                /* Save 'last' before memmove operation and implicitly unput
                 * it (i.e. don't increment p) so that scanning can resume at
                 * the right position. */
                last = *p;

                memmove(p - 1, p, rlen);
                p[rlen - 1] = '\0';

                continue;
            }
        }

        last = *p++;
    }

    dbg_err_if (u_string_set(line, ln, strlen(ln)));

    return 0;
err:
    return ~0;
}

