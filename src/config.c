/* 
 * Copyright (c) 2005, 2006 by KoanLogic s.r.l. - All rights reserved.  
 */

static const char rcsid[] =
    "$Id: config.c,v 1.14 2006/08/07 10:31:21 tho Exp $";

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include <u/carpal.h>
#include <u/queue.h>
#include <u/config.h>
#include <u/misc.h>
#include <u/memory.h>
#include <u/str.h>

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

/**
 * \brief  Print configuration to \c stdout
 *
 * Print a configuration object and its children to \c stdout. For each config
 * object all keys/values pair will be printed.
 *
 * \param c     configuration object
 * \param lev   nesting level; must be zero
 *
 * \return \c 0 on success, not zero on failure
 */
void u_config_print(u_config_t *c, int lev)
{
    u_config_t *item;
    int i;

    for(i = 0; i < lev; ++i)
        printf("  ");
    if (c->key)
        printf("%s: %s\n", c->key, c->value == NULL ? "" : c->value);

    ++lev;
    TAILQ_FOREACH(item, &c->children, np)
        u_config_print(item, lev);
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
        if(strcmp(item->key, key) == 0 && n-- == 0)
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

static int u_config_set_value(u_config_t *c, const char *val)
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
    int overwrite)
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
    } else {
        child_key = u_strndup(key, p-key);
        dbg_err_if(child_key == NULL);
        if((child = u_config_get_child(c, child_key)) == NULL)
            dbg_err_if(u_config_add_child(c, child_key, &child));
        U_FREE(child_key);
        return u_config_do_set_key(child, ++p, val, overwrite);
    }
    return 0;
err:
    return ~0;
}

int u_config_add_key(u_config_t *c, const char *key, const char *val)
{
    return u_config_do_set_key(c, key, val, 0 /* don't overwrite */);
}

int u_config_set_key(u_config_t *c, const char *key, const char *val)
{
    return u_config_do_set_key(c, key, val, 1 /* overwrite */);
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
        dbg_err_if(u_string_append(ln, buf, --len));
        if(!u_isnl(buf[len]))
            continue; /* line's longer the bufsz (or eof);get next line chunk */
        else
            break;
    }

    dbg_err_if(p == NULL);

    return 0;
err:
    return ~0;
}

static int u_config_do_load(u_config_t *c, u_config_gets_t cb, void *arg, 
    int overwrite)
{
    enum { MAX_NEST_LEV = 20 };
    u_string_t *line = NULL, *key = NULL, *lastkey = NULL, *value = NULL;
    const char *ln, *p;
    size_t len;
    int lineno = 1;
    u_config_t *child = NULL;

    dbg_err_if(u_string_create(NULL, 0, &line));
    dbg_err_if(u_string_create(NULL, 0, &key));
    dbg_err_if(u_string_create(NULL, 0, &value));
    dbg_err_if(u_string_create(NULL, 0, &lastkey));

    for(; cs_getline(cb, arg, line) == 0; u_string_clear(line), ++lineno)
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
                warn_err("config error [line %d]: { not after a no-value key", 
                         lineno);
            if(!u_isblank_str(++ln))
                warn_err("config error [line %d]: { or } must be the "
                         "only not-blank char in a line", lineno);

            dbg_err_if(u_config_add_child(c, u_string_c(lastkey), &child));
            dbg_err_if(u_config_do_load(child, cb, arg, overwrite));
            dbg_err_if(u_string_clear(lastkey));
            continue;
        } else if(ln[0] == '}') {
            warn_err_ifm(c->parent == NULL,"config error: unmatched '}'");
            if(!u_isblank_str(++ln))
                warn_err("config error [line %d]: { or } must be the "
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
                        overwrite));
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

int u_config_load_from(u_config_t *c, u_config_gets_t cb, 
    void *arg, int overwrite)
{
    dbg_err_if(u_config_do_load(c, cb, arg, overwrite));

    return 0;
err:
    return ~0;
}

static char *u_config_fgetline(void* arg, char * buf, size_t size)
{
    FILE *f = (FILE*)arg;

    return fgets(buf, size, f);
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
    FILE *file;

    /* must dup because fclose() calls close(2) on fd */
    file = fdopen(dup(fd), "r");
    dbg_err_if(file == NULL);

    dbg_err_if(u_config_do_load(c, u_config_fgetline, file, overwrite));

    fclose(file);

    return 0;
err:
    U_FCLOSE(file);
    return ~0;
}

int u_config_load_from_file (const char *file, u_config_t **pc)
{
    int fd = -1;
    u_config_t *c = NULL;

    dbg_return_if (file == NULL, ~0);
    dbg_return_if (pc == NULL, ~0);

    warn_err_if (u_config_create(&c));
    warn_err_if ((fd = open(file, O_RDONLY)) == -1); 
    warn_err_if (u_config_load(c, fd, 0));

    (void) close(fd);

    *pc = c;

    return 0;
err:
    (void) u_config_free(c);
    U_CLOSE(fd);
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

    if((v = u_config_get_subkey_value(c, subkey)) == NULL)
    {
        *out = def;
        return 0;
    }

    for(w = *true_words; *w; ++w)
    {
        if(!strcasecmp(v, w))
        {
            *out = 1;
            return 0; /* ok */
        }
    }

    for(w = *false_words; *w; ++w)
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
 *      \}
 */
