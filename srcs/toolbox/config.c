/* 
 * Copyright (c) 2005-2008 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include <toolbox/carpal.h>
#include <toolbox/queue.h>
#include <toolbox/list.h>
#include <toolbox/config.h>
#include <toolbox/misc.h>
#include <toolbox/memory.h>
#include <toolbox/str.h>

/**
 *  \defgroup config Configuration
 *  \{
 *      \par Intro
 *          The u_config_t object is used to store, load and parse 
 *          text configuration files.
 *          \n
 *          A configuration file is a plain text file that stores a list of
 *          key=values pair in a tree-ordered structure.
 *          TODO
 */

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

/* file system pre-defined driver */
extern u_config_driver_t u_config_drv_fs;
extern u_config_driver_t u_config_drv_mem;

/* forward declarations */
static int u_config_include(u_config_t*, u_config_driver_t*, u_config_t*, int);

/**
 * \brief  Sort config children 
 *
 * Sort config children using the given comparison function. 
 *
 * The comparison function must return an integer less than, equal to, or
 * greater than zero if the first argument is considered to be respectively
 * less than, equal to, or greater than the second.
 *
 * \param c             configuration object
 * \param config_cmp    comparison function
 *
 * \return \c 0 on success, not zero on failure
 */
int u_config_sort_children(u_config_t *c, 
        int(*config_cmp)(u_config_t**, u_config_t**))
{
    u_config_t *child;
    int i, count;
    u_config_t **children = NULL;

    /* count children */
    count = 0;
    TAILQ_FOREACH(child, &c->children, np)
        count++;

    /* create an array of pointers */
    children = u_zalloc(count * sizeof(u_config_t*));
    dbg_err_if(children == NULL);

    /* populate the array of children */
    i = 0;
    TAILQ_FOREACH(child, &c->children, np)
        children[i++] = child;

    /* sort the list */
    qsort(children, count, sizeof(u_config_t*), 
            (int(*)(const void*,const void*))config_cmp);

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
 * \brief  Print configuration to the given file descriptor
 *
 * Print a configuration object and its children to \c fp. For each config
 * object all keys/values pair will be printed.
 *
 * \param c     configuration object
 * \param fp    output file descriptor
 * \param lev   nesting level; must be zero
 *
 * \return nothing
 */
void u_config_print_to_fp(u_config_t *c, FILE *fp, int lev)
{
    u_config_t *item;
    int i;

    for(i = 0; i < lev; ++i)
        fprintf(fp, "  ");

    if (c->key)
        fprintf(fp, "%s  %s\n", c->key, c->value == NULL ? "" : c->value);

    ++lev;
    TAILQ_FOREACH(item, &c->children, np)
        u_config_print_to_fp(item, fp, lev);
}

/**
 * \brief  Print configuration to \c stdout
 *
 * Print a configuration object and its children to \c stdout. For each config
 * object all keys/values pair will be printed.
 *
 * \param c     configuration object
 * \param lev   nesting level; must be zero
 *
 * \return nothing
 */
void u_config_print(u_config_t *c, int lev)
{
    u_config_print_to_fp(c, stdout, lev);
    return;
}

int u_config_add_child(u_config_t *c, const char *key, u_config_t **pc)
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

/* get n-th child item called key */
u_config_t* u_config_get_child_n(u_config_t *c, const char *key, int n)
{
    u_config_t *item;

    TAILQ_FOREACH(item, &c->children, np)
    {
        if((key == NULL || strcmp(item->key, key) == 0) && n-- == 0)
            return item;  /* found */
    }

    return NULL; /* not found */
}

u_config_t* u_config_get_child(u_config_t *c, const char *key)
{
    return u_config_get_child_n(c, key, 0);
}

int u_config_get_subkey_nth(u_config_t *c, const char *subkey, int n, 
    u_config_t **pc)
{
    u_config_t *child = NULL;
    char *first_key = NULL, *p;

    if((p = strchr(subkey, '.')) == NULL)
    {
        if((child = u_config_get_child_n(c, subkey, n)) != NULL)
        {
            *pc = child;
            return 0;
        } 
    } else {
        if((first_key = u_strndup(subkey, p-subkey)) != NULL)
        {
            child = u_config_get_child(c, first_key);
            U_FREE(first_key);
        }
        if(child != NULL)
            return u_config_get_subkey(child, ++p, pc);
    }
    return ~0; /* not found */

}

int u_config_get_subkey(u_config_t *c, const char *subkey, u_config_t **pc)
{
    return u_config_get_subkey_nth(c, subkey, 0, pc);
}

static u_config_t* u_config_get_root(u_config_t *c)
{
    while(c->parent)
        c = c->parent;
    return c;
}

int u_config_set_value(u_config_t *c, const char *val)
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

            /* look for closig bracket */
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

int u_config_add_key(u_config_t *c, const char *key, const char *val)
{
    return u_config_do_set_key(c, key, val, 0 /* don't overwrite */, NULL);
}

int u_config_set_key(u_config_t *c, const char *key, const char *val)
{
    return u_config_do_set_key(c, key, val, 1 /* overwrite */, NULL);
}

static int cs_getline(u_config_gets_t cb, void *arg, u_string_t *ln)
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

static int u_config_do_load_drv(u_config_t *c, u_config_driver_t *drv, 
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
        /* remove comments if any */
        if((p = strchr(u_string_c(line), '#')) != NULL)
            dbg_err_if(u_string_set_length(line, p - u_string_c(line)));

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

        if(!strcmp(u_string_c(key), "include") ||       /* forced dependency */
                !strcmp(u_string_c(key), "-include"))   /* optional dependency */
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

static int u_config_include(u_config_t *c, u_config_driver_t *drv, 
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

    crit_err_ifm ( drv->open == NULL,
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
        warn("the 'close' driver callback is not defined, not closing...");

    return 0;
err:
    if(arg && drv->close)
        drv->close(arg);
    return ~0;
}

/**
 * \brief  Delete a child config object.
 *
 *  Delete a child config object. \c child must be a child of \c c.
 *
 * \param c         configuration object
 * \param child     the config object to delete
 *
 */
static void u_config_del_key(u_config_t *c, u_config_t *child)
{
    TAILQ_REMOVE(&c->children, child, np);
}

/**
 * \brief  Load from an opaque data source
 *
 *  Load configuration data from a data source; a gets()-like function must
 *  be provided by the caller to read the configuration line-by-line.
 *
 *  Using this interface is not possibile to use the 'include' command; use
 *  u_config_load_from_file or u_config_load_from_drv instead.
 *
 * \param c         configuration object
 * \param cb        gets()-like callback
 * \param arg       opaque argument passed to the 'cb' function
 * \param overwrite     if 1 overwrite keys with the same name otherwise new
 *
 */
int u_config_load_from(u_config_t *c, u_config_gets_t cb, 
    void *arg, int overwrite)
{
    u_config_driver_t drv = { NULL, NULL, cb, NULL };

    dbg_err_if(u_config_do_load_drv(c, &drv, arg, overwrite));

    return 0;
err:
    return ~0;
}

/**
 * \brief  Load a configuration file.
 *
 *  Fill a config object with key/value pairs loaded from the configuration
 *  file linked to the descriptor \c fd. If \c overwrite is not zero 
 *  values of keys with the same name will overwrite previous values otherwise
 *  new keys with the same name will be added.
 *
 * \param c             configuration object
 * \param fd            file descriptor 
 * \param overwrite     if 1 overwrite keys with the same name otherwise new
 *                      key with the same name will be added
 *
 * \return \c 0 on success, not-zero on error.
 */
int u_config_load(u_config_t *c, int fd, int overwrite)
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
 * \brief  Create a configuration object reading from a config file
 *
 * Create a configuration object reading from a config file
 *
 * \param file      file descriptor 
 * \param pc        value-result that will get the new configuration object
 *
 * \return \c 0 on success, not-zero on error.
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
 * \brief  Create a config object.
 *
 *  Create a config object. Use u_config_set_key(...) to set its key/value 
 *  pairs.
 *
 * \param pc        value-result that will get the new configuration object
 *
 * \return \c 0 on success, not-zero on error.
 */
int u_config_create(u_config_t **pc)
{
    u_config_t *c = NULL;

    c = u_zalloc(sizeof(u_config_t));
    dbg_err_if(c == NULL);

    TAILQ_INIT(&c->children);

    *pc = c;

    return 0;
err:
    if(c)
        u_config_free(c);
    return ~0;
}

/**
 * \brief  Remove a child (and all its sub-children) from a config object
 *
 * Remove a child and  all its sub-children from a config object.
 *
 * \param c     configuration object
 * \param child     child to remove
 *
 * \return \c 0 on success, not zero on failure
 */
int u_config_del_child(u_config_t *c, u_config_t *child)
{
    u_config_t *item;

    TAILQ_FOREACH(item, &c->children, np)
    {
        if(item == child)
        {   /* found */
            u_config_del_key(c, child);
            dbg_err_if(u_config_free(child));
            return 0;
        }
    }

err:
    return ~0;
}

/**
 * \brief  Free a config object and all its children.
 *
 *  Free a config object and all its children.
 *
 * \param c     configuration object
 *
 * \return \c 0 on success, not zero on failure
 */
int u_config_free(u_config_t *c)
{
    u_config_t *child = NULL;
    if(c)
    {
        /* free all children */
        while((child = TAILQ_FIRST(&c->children)) != NULL)
        {
            u_config_del_key(c, child);
            dbg_err_if(u_config_free(child));
        }
        /* free parent */
        if(c->key)
            U_FREE(c->key);
        if(c->value)
            U_FREE(c->value);
        U_FREE(c);
    }
    return 0;
err:
    return ~0;
}

/**
 * \brief  Return the key of a config object.
 *
 *  Return the key string of a config object.
 *
 * \param c         configuration object
 *
 * \return The key string pointer on success, \c NULL on failure
 */
const char* u_config_get_key(u_config_t *c)
{
    dbg_err_if(!c);

    return c->key;
err:
    return NULL;
}

/**
 * \brief  Return the value of a config object.
 *
 *  Return the value string of a config object.
 *
 * \param c         configuration object
 *
 * \return The value string pointer on success, \c NULL on failure
 */
const char* u_config_get_value(u_config_t *c)
{
    dbg_err_if(!c);
    
    return c->value;
err:
    return NULL;
}

/**
 * \brief  Return the value of a subkey.
 *
 *  Return the value of the child config object whose key is \c subkey.
 *
 * \param c         configuration object
 * \param subkey    the key value of the child config object
 *
 * \return The value string pointer on success, \c NULL on failure
 */
const char* u_config_get_subkey_value(u_config_t *c, const char *subkey)
{
    u_config_t *skey;

    nop_err_if(u_config_get_subkey(c, subkey, &skey));

    return u_config_get_value(skey);
err:
    return NULL;
}

/**
 * \brief  Return the value of an integer subkey.
 *
 *  Return the integer value (atoi is used for conversion) of the child config 
 *  object whose key is \c subkey.
 *
 * \param c         configuration object
 * \param subkey    the key value of the child config object
 * \param def       the value to return if the key is missing
 * \param out       on exit will get the integer value of the key
 *
 * \return 0 on success, not zero if the key value is not an int
 */
int u_config_get_subkey_value_i(u_config_t *c, const char *subkey, int def, 
    int *out)
{
    const char *v;
    char *ep = NULL;
    long l;

    if((v = u_config_get_subkey_value(c, subkey)) == NULL)
    {
        *out = def; 
        return 0;
    }

    l = strtol(v, &ep, 0);
    dbg_err_if(*ep != '\0'); /* dirty int value (for ex. 123text) */

    /* same cast used by atoi */
    *out = (int)l;

    return 0;
err:
    return ~0;
}

/**
 * \brief  Return the value of an bool subkey.
 *
 *  Return the bool value of the child config object whose key is \c subkey.
 *
 *  Recognized bool values are (case insensitive) yes/no, 1/0, enable/disable.
 *
 * \param c         configuration object
 * \param subkey    the key value of the child config object
 * \param def       the value to return if the key is missing 
 * \param out       on exit will get the bool value of the key
 *
 * \return 0 on success, not zero if the key value is not a bool keyword
 */
int u_config_get_subkey_value_b(u_config_t *c, const char *subkey, int def,  
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

int u_config_load_from_drv(const char *uri, u_config_driver_t *drv, 
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

int u_config_has_children(u_config_t *c)
{
    return (TAILQ_FIRST(&c->children) != NULL);
}

static int u_config_to_str(u_config_t *c, u_string_t *s)
{
    u_config_t *item;

    dbg_err_if(c == NULL);
    dbg_err_if(s == NULL);

    if(c->key)
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

int u_config_save_to_buf(u_config_t *c, char *buf, size_t size)
{
    u_string_t *s = NULL;

    dbg_err_if(u_string_create(NULL, 0, &s));

    dbg_err_if(u_config_to_str(c, s));

    /* buffer too small */
    warn_err_if(u_string_len(s) >= size);

    strlcpy(buf, u_string_c(s), size);

    u_string_free(s);

    return 0;
err:
    if(s)
        u_string_free(s);
    return ~0;
}

/* gets() callback helper struct */
struct u_config_buf_s
{
    char *data;
    size_t len;
};

static char* u_config_buf_gets(void *arg, char *buf, size_t size)
{
    struct u_config_buf_s *g = (struct u_config_buf_s*)arg;
    char *s = buf;
    int c;

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

int u_config_load_from_buf(char *buf, size_t len, u_config_t **pc)
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
 *      \}
 */

