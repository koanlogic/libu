/*
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.
 */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <toolbox/memory.h>
#include <toolbox/carpal.h>
#include <toolbox/hmap.h>
#include <toolbox/queue.h>
#include <toolbox/str.h>
#include <toolbox/misc.h>

/* default limits handled by policies */
#define U_HMAP_MAX_SIZE      512
#define U_HMAP_MAX_ELEMS     U_HMAP_MAX_SIZE
#define U_HMAP_RATE_FULL     0.75
#define U_HMAP_RATE_RESIZE   3

/* types of operation used for handling policies */
enum {
    U_HMAP_PCY_OP_PUT = 0x1,
    U_HMAP_PCY_OP_GET = 0x2
};

/* policy queue object */
struct u_hmap_q_s
{
    u_hmap_o_t *ho;     /**< reference to hmap object */
    void *data;         /**< internal policy data */

    TAILQ_ENTRY(u_hmap_q_s) next;
};
typedef struct u_hmap_q_s u_hmap_q_t;

/* hmap policy representation */
struct u_hmap_pcy_s
{
    int (*pop)(u_hmap_t *hmap, u_hmap_o_t **obj);
    int (*push)(u_hmap_t *hmap, u_hmap_o_t *obj,
            u_hmap_q_t **data);
    int ops;    /* bitwise inclusive OR of U_HMAP_PCY_OP_* values */

    TAILQ_HEAD(u_hmap_q_h_s, u_hmap_q_s) queue;
};
typedef struct u_hmap_q_h_s u_hmap_q_h_t;

/* hmap element object */
struct u_hmap_o_s
{
    void *key;
    void *val;

    LIST_ENTRY(u_hmap_o_s) next;

    u_hmap_q_t *pqe;

    u_hmap_t *hmap;
};

/* Map options */
struct u_hmap_opts_s
{
    size_t size,            /**< approximate size of hashmap array */
           max;             /**< maximum number of elements in hmap -
                             only applies to hmaps with discard policy */

    u_hmap_type_t type;         /**< type of hashmap */

    u_hmap_pcy_type_t policy;   /**< discard policy (disabled by default) */

    int options;                /**< see definitions for U_HMAP_OPTS_* */

    u_hmap_options_datatype_t key_type;         /**< type of key */
    u_hmap_options_datatype_t val_type;         /**< type of value */

    size_t key_sz;                      /* size of key (if OPAQUE) */
    size_t val_sz;                      /* size of value (if OPAQUE) */

    /** hash function to be used in hashhmap */
    size_t (*f_hash)(const void *key, size_t buckets);
    /** function for key comparison */
    int (*f_comp)(const void *k1, const void *k2);
    /** function for freeing an object */
    void (*f_free)(u_hmap_o_t *obj);
    /** function for freeing a key */
    void (*f_key_free)(const void *key);
    /** function for freeing a value */
    void (*f_val_free)(void *val);
    /** function to get a string representation of a (key, val) object */
    u_string_t *(*f_str)(u_hmap_o_t *obj);
    /** custom policy comparison function */
    int (*f_pcy_cmp)(void *o1, void *o2);

    unsigned char easy;         /**< whether simplified interface is active
                                  (internal) */
    unsigned char val_free_set; /**< whether value free function has been set -
                                  used in easy interface to force the call
                                  (internal) */
};

/* hmap representation */
struct u_hmap_s
{
    u_hmap_opts_t *opts;        /* hmap options */

    size_t sz,                  /* current size */
           size,                /* array size */
           threshold,           /* when to resize */
           px;                  /* index into prime numbers array */

    u_hmap_pcy_t pcy;    /* discard policy */

    LIST_HEAD(u_hmap_e_s, u_hmap_o_s) *hmap;    /* the hashmap */
};
typedef struct u_hmap_e_s u_hmap_e_t;

static int __get (u_hmap_t *hmap, const void *key, u_hmap_o_t **o);

static int __opts_check (u_hmap_opts_t *opts);
static int __pcy_setup (u_hmap_t *hmap);
static const char *__pcy2str(u_hmap_pcy_type_t policy);

static void __o_free (u_hmap_t *hmap, u_hmap_o_t *obj);
static int __o_overwrite (u_hmap_t *hmap, u_hmap_o_t *o, u_hmap_o_t *obj,
        u_hmap_o_t **old);

static u_hmap_q_t *__q_o_new (u_hmap_o_t *ho);
static void __q_o_free (u_hmap_q_t *s);

static size_t __f_hash (const void *key, size_t size);
static int __f_comp (const void *k1, const void *k2);
static u_string_t *__f_str (u_hmap_o_t *obj);

static int __queue_push (u_hmap_t *hmap, u_hmap_o_t *obj, u_hmap_q_t **data);
static int __queue_push_count (u_hmap_t *hmap, u_hmap_o_t *obj,
        u_hmap_q_t **counts);
static int __queue_push_cmp (u_hmap_t *hmap, u_hmap_o_t *obj,
        u_hmap_q_t **counts);
static int __queue_pop_front (u_hmap_t *hmap, u_hmap_o_t **obj);
static int __queue_pop_back (u_hmap_t *hmap, u_hmap_o_t **obj);
static int __queue_del (u_hmap_t *hmap, u_hmap_o_t *obj);

static int __resize(u_hmap_t *hmap);
static int __next_prime(size_t *prime, size_t sz, size_t *idx);

static const char *__datatype2str(u_hmap_options_datatype_t datatype);

/**
 *  \defgroup hmap HMap
 *  \{
 *      \par
 *      The \ref hmap module provides a flexible hash-map implementation
 *      featuring:
 *          - hash bucket chaining or linear probing \link hmap::u_hmap_type_t
 *          operating modes\endlink;
 *          - pointer, string or opaque \link hmap::u_hmap_options_datatype_t
 *          data types\endlink for both keys and values;
 *          - optional cache-style usage by specifying \link
 *          hmap::u_hmap_pcy_type_t discard policies\endlink (FIFO, LRU, LFU
 *          and CUSTOM);
 *          - choice between user or hmap memory \link hmap::u_hmap_options_t
 *          ownership\endlink;
 *          - custom hash function, comparison function and object free
 *          functions.
 *
 *      \par Simplified Interface
 *      Developers new to hmap or those wanting a plain and simple
 *      { string -> object } hashmap implementation are strongly recommended to
 *      use the new \ref hmap_easy "hmap easy" interface. It is more
 *      user-friendly than the standard version and still allows choice between
 *      operating modes, policies and other options applied to hmap values.
 *
 *      \par
 *      The following examples illustrates the basic usage of the hmap_easy
 *      interface:
 *
 *      \code
 *          u_hmap_opts_t *opts = NULL;
 *          u_hmap_t *hmap = NULL;
 *
 *          dbg_err_if (u_hmap_opts_new(&opts));
 *          dbg_err_if (u_hmap_opts_set_val_type(opts,
 *                          U_HMAP_OPTS_DATATYPE_STRING));
 *          dbg_err_if (u_hmap_easy_new(opts, &hmap));
 *
 *          dbg_err_if (u_hmap_easy_put(hmap, "jack", (const void *) ":S"));
 *          dbg_err_if (u_hmap_easy_put(hmap, "jill", (const void *) ":)))"));
 *
 *          u_con("jack is %s and jill is %s",
 *              (const char *) u_hmap_easy_get(hmap, "jack"),
 *              (const char *) u_hmap_easy_get(hmap, "jill"));
 *      err:
 *          U_FREEF(hmap, u_hmap_easy_free);
 *          U_FREEF(opts, u_hmap_opts_free);
 *      \endcode
 *
 *      As illustrated by the example above, keys are always strings in the easy
 *      interface, but there are three types of possibile value types, which can
 *      be set by using \ref hmap::u_hmap_opts_set_val_type.
 *      - U_HMAP_OPTS_DATATYPE_STRING (as in the example above): the hmap knows
 *      how to handle allocation and deallocation of strings, so you do not need
 *      to set any free functions. The user can simply insert string values
 *      (even static) into the hashmap and the hmap will worry about
 *      internalisation;
 *      - U_HMAP_OPTS_DATATYPE_OPAQUE: in a similar manner to strings, hmap
 *      automatically handles internalisation of opaque data. The only extra
 *      information necessary is the fixed size of the opaque values within the
 *      hmap, which can be set using u_hmap_opts_set_val_sz. If not set, the
 *      default size will be the size of a pointer - i.e. <tt>sizeof(void
 *      *)</tt>;
 *      - U_HMAP_OPTS_DATATYPE_POINTER (default): allows the user to insert
 *      pointers to any generic object into the hashmap. In this case the user
 *      MUST set a free function for his/her own object type by using \ref
 *      hmap::u_hmap_opts_set_val_freefunc (the value MAY be NULL, useful for
 *      pointers to static data which does not require deallocation). If the
 *      value set is not NULL, the hmap will call the free function on objects
 *      removed from the hmap (via deletion, overwrite or hmap object
 *      deallocation).
 *
 *      \par Discard Policies for Caching
 *      HMap also features a useful caching component which allows the user to
 *      specify a maximum size after which elements are discarded using one of
 *      the following policies:
 *          - U_HMAP_PCY_NONE: the default policy is to never discard elements;
 *          - U_HMAP_PCY_FIFO: First In First Out discard policy;
 *          - U_HMAP_PCY_LRU: Least Recently Used discard policy;
 *          - U_HMAP_PCY_LFU: Least Frequently Used discard policy;
 *          - U_HMAP_PCY_CUSTOM: Custom discard policy.
 *      For a basic illustration of how each of these work, take a look at
 *      the source code of the \ref hmap::u_hmap_pcy_type_t definition.
 *      For a sample usage please refer to test/hmap.c.
 *
 *      \par Advanced Interface
 *      The advanced interface is required <em>if and only if</em>:
 *          - you want to able to customise the key type to be different than
 *          the default C-style string;
 *          - for some reason you cannot let hmap manage deallocations for you
 *          (by calling your custom handler);
 *          - you don't like the way overwrites are handled in the easy
 *          interface (overwritten values are not returned automatically, they
 *          must be retrieved by using \ref hmap::u_hmap_easy_get).
 *
 *      \par
 *      The \ref hmap_easy "easy interface" should do just fine for most
 *      applications and it is less error-prone than the \ref hmap_full
 *      "advanced interface".
 *      So, if you must use the latter, please take special care.
 *
 *      More examples of both easy and advanced API usage can be found in
 *      <tt>test/hmap.c</tt>.
 */

/**
 *      \defgroup hmap_easy hmap_easy: Simplified Interface (recommended)
 *      \{
 */

/**
 * \brief   Create a new hmap
 *
 * Create a new hmap object and save its pointer to \p *hmap.
 * The call may fail on memory allocation problems or if the options are
 * manipulated incorrectly.
 *
 * \param   opts      options to be passed to the hmap
 * \param   phmap     on success contains the hmap options object
 *
 * \retval  U_HMAP_ERR_NONE     on success
 * \retval  U_HMAP_ERR_FAIL     on failure
 */
int u_hmap_easy_new (u_hmap_opts_t *opts, u_hmap_t **phmap)
{
    u_hmap_opts_t newopts;

    dbg_err_if (opts == NULL);
    dbg_err_if (phmap == NULL);

    u_hmap_opts_init(&newopts);

    dbg_err_if (u_hmap_opts_copy(&newopts, opts));

    /* same defaults as advanced interface, but mark as easy */
    newopts.easy = 1;

    dbg_err_if (u_hmap_new(&newopts, phmap));

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/**
 * \brief   Clear hmap
 *
 * See u_hmap_clear().
 *
 * \param   hmap      hmap object
 */
void u_hmap_easy_clear (u_hmap_t *hmap)
{
    dbg_ifb (hmap == NULL) return;

    u_hmap_clear(hmap);
}

/**
 * \brief   Deallocate hmap
 *
 * Deallocate \p hmap along with all hmapd objects. Objects are freed by using
 * the function pointer specified with u_hmap_opts_set_val_freefunc() if it is
 * not NULL.
 *
 * \param   hmap      hmap object
 */
void u_hmap_easy_free (u_hmap_t *hmap)
{
    dbg_return_if (hmap == NULL, );

    u_hmap_free(hmap);

    return;
}

/**
 * \brief   Insert an object into the hmap
 *
 * Insert a \p key, \p val pair into \p hmap.
 *
 * The size of \p val copied into the hmap is based on its type set using
 * u_hmap_opts_set_val_type():
 *      - U_HMAP_OPTS_DATATYPE_POINTER (default): the pointer value will be
 *      simply copied
 *      - U_HMAP_OPTS_DATATYPE_STRING: the null-terminated string value will
 *      copied entirely
 *      - U_HMAP_OPTS_DATATYPE_OPAQUE: the value will be duplicated to the size
 *      set with u_hmap_opts_set_val_sz()
 *
 * Values are not overwritten if a value already exists in the hmap for a given
 * \p key, unless U_HMAP_OPTS_NO_OVERWRITE is explicitly unset using
 * u_hmap_opts_unset_option().
 *
 * \param   hmap      hmap object
 * \param   key       key to be inserted
 * \param   val       value to be inserted
 *
 * \retval  U_HMAP_ERR_NONE     on success
 * \retval  U_HMAP_ERR_EXISTS   if key already exists
 * \retval  U_HMAP_ERR_FAIL     on other failures
 */
int u_hmap_easy_put (u_hmap_t *hmap, const char *key, const void *val)
{
    int rc = U_HMAP_ERR_NONE;
    u_hmap_o_t *obj = NULL;

    dbg_err_if ((obj = u_hmap_o_new(hmap, key, val)) == NULL);
    dbg_err_if ((rc = u_hmap_put(hmap, obj, NULL)));

    return rc;
err:
    /* don't free 'obj' because hmap owns it and will free it */
    return (rc ? rc : U_HMAP_ERR_FAIL);
}

/**
 * \brief   Retrieve a value from the hmap
 *
 * Retrieve the value with given \p key from \p hmap.
 *
 * \param   hmap      hmap object
 * \param   key       key to be retrieved
 *
 * \return  a pointer to the retrieved value on success, \c NULL on failure or
 *          no match
 */
void *u_hmap_easy_get (u_hmap_t *hmap, const char *key)
{
    u_hmap_o_t *obj = NULL;

    nop_return_if (u_hmap_get(hmap, key, &obj), NULL);

    return obj->val;
}

/**
 * \brief   Delete an object from the hmap
 *
 * Delete object with given \p key from \p hmap.
 *
 * \param   hmap      hmap object
 * \param   key       key of object to be deleted
 *
 * \retval  U_HMAP_ERR_NONE     on success
 * \retval  U_HMAP_ERR_FAIL     on failure
 */
int u_hmap_easy_del (u_hmap_t *hmap, const char *key)
{
    return (u_hmap_del(hmap, key, NULL));
}

/**
 *      \}
 */

/* Default hash function */
static size_t __f_hash (const void *key, size_t size)
{
    size_t h = 0;
    const unsigned char *k = (const unsigned char *) key;

    dbg_ifb (key == NULL) return -1;

    while (*k)
    {
        h += *k++;
        h += (h << 10);
        h ^= (h >> 6);
    }

    h += (h << 3);
    h ^= (h >> 11);

    return (h + (h << 15)) % size;
}

/* Default comparison function for key comparison */
static int __f_comp (const void *k1, const void *k2)
{
    dbg_ifb (k1 == NULL) return -1;
    dbg_ifb (k2 == NULL) return -1;

    return strcmp((const char *)k1, (const char *)k2);
}

/* Default string representation of objects */
static u_string_t *__f_str (u_hmap_o_t *obj)
{
    enum { MAX_OBJ_STR = 256 };
    char buf[MAX_OBJ_STR];
    u_string_t *s = NULL;
    const char *key, *val;

    dbg_err_if (obj == NULL);

    key = (obj->hmap->opts->key_type == U_HMAP_OPTS_DATATYPE_STRING) ?
        (const char *) obj->key : "";
    val = (obj->hmap->opts->val_type == U_HMAP_OPTS_DATATYPE_STRING) ?
        (const char *) obj->val : "";

    dbg_err_if (u_snprintf(buf, MAX_OBJ_STR, "[%s:%s]", key, val));
    dbg_err_if (u_string_create(buf, strlen(buf)+1, &s));

    return s;

err:
    U_FREE(s);
    return NULL;
}

/* Check validity of options */
static int __opts_check (u_hmap_opts_t *opts)
{
    dbg_err_if (opts == NULL);

    dbg_err_if (opts->size == 0);
    dbg_err_if (opts->max == 0);
    dbg_err_if (opts->type != U_HMAP_TYPE_CHAIN &&
            opts->type != U_HMAP_TYPE_LINEAR);
    dbg_err_if (!U_HMAP_IS_PCY(opts->policy));
    dbg_err_if (opts->f_hash == NULL);
    dbg_err_if (opts->f_comp == NULL);

    /* in the hmap_easy interface, in case of pointer values (default),
       we force setting the value free function to avoid developer mistakes;
       for string or opaque values there is no need because we already know how
       to handle them */
    dbg_err_ifm (opts->easy &&
            opts->val_type == U_HMAP_OPTS_DATATYPE_POINTER &&
            !opts->val_free_set,
                "value free function must be set for pointers!");

    dbg_err_ifm (opts->policy == U_HMAP_PCY_CUSTOM &&
            opts->f_pcy_cmp == NULL,
                "comparison function must be set for custom policy!");

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/* Setup policy parameters */
static int __pcy_setup (u_hmap_t *hmap)
{
    dbg_return_if (hmap == NULL, ~0);

    switch (hmap->opts->policy)
    {
        case U_HMAP_PCY_NONE:
            hmap->pcy.push = NULL;
            hmap->pcy.pop = NULL;
            hmap->pcy.ops = 0;
            break;
        case U_HMAP_PCY_FIFO:
            hmap->pcy.push = __queue_push;
            hmap->pcy.pop = __queue_pop_back;
            hmap->pcy.ops = U_HMAP_PCY_OP_PUT;
            break;
        case U_HMAP_PCY_LRU:
            hmap->pcy.push = __queue_push;
            hmap->pcy.pop = __queue_pop_back;
            hmap->pcy.ops = U_HMAP_PCY_OP_PUT | U_HMAP_PCY_OP_GET;
            break;
        case U_HMAP_PCY_LFU:
            hmap->pcy.push = __queue_push_count;
            hmap->pcy.pop = __queue_pop_front;
            hmap->pcy.ops = U_HMAP_PCY_OP_PUT | U_HMAP_PCY_OP_GET;
            break;
        case U_HMAP_PCY_CUSTOM:
            hmap->pcy.push = __queue_push_cmp;
            hmap->pcy.pop = __queue_pop_back;
            hmap->pcy.ops = U_HMAP_PCY_OP_PUT;
            break;
        default:
            u_dbg("Invalid policy: %d", hmap->opts->policy);
            return U_HMAP_ERR_FAIL;
    }

    return U_HMAP_ERR_NONE;
}

/**
 *      \defgroup hmap_full hmap: Advanced Interface
 *      \{
 */

/**
 * \brief   Create a new hmap
 *
 * Create a new hmap object and save its pointer to \p *hmap.
 * The call may fail on memory allocation problems or if the options are
 * manipulated incorrectly.
 *
 * \param   opts      options to be passed to the hmap (may be NULL)
 * \param   phmap     on success contains the hmap options object
 *
 * \retval  U_HMAP_ERR_NONE     on success
 * \retval  U_HMAP_ERR_FAIL     on failure
 */
int u_hmap_new (u_hmap_opts_t *opts, u_hmap_t **phmap)
{
    size_t i;
    u_hmap_t *c = NULL;

    /* allow (opts == NULL) */
    dbg_return_if (phmap == NULL, U_HMAP_ERR_FAIL);

    dbg_return_sif ((c = (u_hmap_t *) u_zalloc(sizeof(u_hmap_t))) == NULL,
            U_HMAP_ERR_FAIL);

    dbg_err_if (u_hmap_opts_new(&c->opts));
    if (opts)
        dbg_err_if (u_hmap_opts_copy(c->opts, opts));

    /* if key or value are strings we know how to display them */
    if (c->opts->f_str == NULL &&
            (c->opts->key_type == U_HMAP_OPTS_DATATYPE_STRING ||
            c->opts->val_type == U_HMAP_OPTS_DATATYPE_STRING))
        c->opts->f_str = &__f_str;

    dbg_err_if (__opts_check(c->opts));
    u_hmap_opts_dbg(c->opts);
    dbg_err_if (__pcy_setup(c));

    c->size = c->opts->size;
    dbg_err_if (__next_prime(&c->size, c->size, &c->px));
    c->threshold = (size_t) (U_HMAP_RATE_FULL * c->size);

    dbg_err_sif ((c->hmap = (u_hmap_e_t *)
                u_zalloc(sizeof(u_hmap_e_t) *
                    c->size)) == NULL);

    /* initialise entries */
    for (i = 0; i < c->size; ++i)
        LIST_INIT(&c->hmap[i]);

    TAILQ_INIT(&c->pcy.queue);

    u_dbg("[hmap]");
    u_dbg("threshold: %u", c->threshold);

    *phmap = c;

    return U_HMAP_ERR_NONE;

err:
    u_free(c);
    *phmap = NULL;
    return U_HMAP_ERR_FAIL;
}

/**
 * \brief   Clear hmap
 *
 * Clears all hmap elements. Objects are freed via free() by default or using
 * the custom deallocation function passed in the hmap options.
 *
 * \param   hmap      hmap object
 */
void u_hmap_clear (u_hmap_t *hmap)
{
    u_hmap_o_t *obj;
    u_hmap_q_t *data;
    size_t i;

    dbg_ifb (hmap == NULL) return;

    /* free the hashhmap */
    for (i = 0; i < hmap->size; ++i)
    {
        while ((obj = LIST_FIRST(&hmap->hmap[i])) != NULL)
        {
            LIST_REMOVE(obj, next);
            __o_free(hmap, obj);
        }
    }

    /* free the policy queue */
    while ((data = TAILQ_FIRST(&hmap->pcy.queue)) != NULL)
    {
        TAILQ_REMOVE(&hmap->pcy.queue, data, next);
        __q_o_free(data);
    }
}

/**
 * \brief   Deallocate hmap
 *
 * Deallocate \p hmap along with all hmapd objects (unless U_HMAP_OPTS_OWNSDATA
 * is set). Objects are freed via free() by default or using the custom
 * deallocation function passed in the hmap options.
 *
 * \param   hmap      hmap object
 */
void u_hmap_free (u_hmap_t *hmap)
{
    dbg_ifb (hmap == NULL) return;

    u_hmap_clear(hmap);
    u_free(hmap->hmap);
    u_hmap_opts_free(hmap->opts);
    u_free(hmap);
}

/**
 * \brief   Insert an object into the hmap
 *
 * Insert a (key, val) pair \p obj into \p hmap. Such object should be created
 * with u_hmap_o_new(). The user is responsible for allocation of keys and
 * values unless U_HMAP_OPTS_OWNSDATA is set. If a value is overwritten
 * (U_HMAP_OPTS_NO_OVERWRITE must be unset via u_hmap_opts_unset_option()) and
 * data is owned by the user, the \p old value is returned.
 *
 * \param   hmap      hmap object
 * \param   obj       key to be inserted
 * \param   old       returned old value
 *
 * \retval  U_HMAP_ERR_NONE     on success
 * \retval  U_HMAP_ERR_EXISTS   if key already exists
 * \retval  U_HMAP_ERR_FAIL     on other failures
 */
int u_hmap_put (u_hmap_t *hmap, u_hmap_o_t *obj, u_hmap_o_t **old)
{
    u_hmap_o_t *o;
    u_hmap_e_t *x;
    int comp;
    int rc;
    size_t hash;
    size_t last;

    dbg_err_if (hmap == NULL);
    dbg_err_if (obj == NULL);

    if (old)
        *old = NULL;

    if (hmap->sz >= hmap->threshold &&
            hmap->opts->policy == U_HMAP_PCY_NONE)
        dbg_err_if (__resize(hmap));

    hash = hmap->opts->f_hash(obj->key, hmap->size);

    /* rehash if strong hash is required */
    if (hmap->opts->f_hash != &__f_hash &&
            !(hmap->opts->options & U_HMAP_OPTS_HASH_STRONG))
    {
        enum { MAX_INT = 20 };
        char h[MAX_INT];

        u_snprintf(h, MAX_INT, "%u", hash);
        hash = __f_hash(h, hmap->size);
    }

    x = &hmap->hmap[hash];

    switch (hmap->opts->type)
    {
        case U_HMAP_TYPE_CHAIN:

            if (LIST_EMPTY(x))
            {
                LIST_INSERT_HEAD(x, obj, next);
                goto end;
            }

            LIST_FOREACH(o, x, next)
            {
                /* object already hmapd */
                if ((comp = hmap->opts->f_comp(obj->key, o->key)) == 0)
                {
                    rc = __o_overwrite(hmap, o, obj, old);
                    dbg_err_if (rc && rc != U_HMAP_ERR_EXISTS);
                    return rc;
                }
                else
                {
                    if (comp < 0)
                    {
                        LIST_INSERT_BEFORE(o, obj, next);
                        goto end;
                    }
                    else if (!LIST_NEXT(o, next))
                    {
                        LIST_INSERT_AFTER(o, obj, next);
                        goto end;
                    }
                }
            }
            break;

        case U_HMAP_TYPE_LINEAR:

            last = ((hash + hmap->size-1) % hmap->size);

            for (; hash != last; hash = ((hash+1) % hmap->size),
                    x = &hmap->hmap[hash])
            {
                if (LIST_EMPTY(x))
                {
                    LIST_INSERT_HEAD(x, obj, next);
                    goto end;
                }

                /* only first node used for linear probing */
                o = LIST_FIRST(x);

                /* object already hmapd */
                if (hmap->opts->f_comp(o->key, obj->key) == 0)
                {
                    rc = __o_overwrite(hmap, o, obj, old);
                    dbg_err_if (rc && rc != U_HMAP_ERR_EXISTS);
                    return rc;
                }
            }
            break;
    }

err:
    return U_HMAP_ERR_FAIL;

end:

    if (hmap->sz >= hmap->opts->max &&
            hmap->opts->policy != U_HMAP_PCY_NONE)
    {
        u_dbg("Cache full - freeing according to policy '%s'",
                __pcy2str(hmap->opts->policy));
        dbg_err_if (hmap->pcy.pop(hmap, old));
    }

    if (hmap->pcy.ops & U_HMAP_PCY_OP_PUT)
        dbg_err_if (hmap->pcy.push(hmap, obj, &obj->pqe));
    hmap->sz++;

    return U_HMAP_ERR_NONE;
}

/**
 * \brief   Retrieve an object from the hmap
 *
 * Retrieve object with given \p key from \p hmap. On success the requested
 * object is returned in \p obj. The object is not removed from the hmap, so
 * ownership of the object is not returned to the user.
 *
 * \param   hmap      hmap object
 * \param   key       key to be retrieved
 * \param   obj       returned object
 *
 * \retval  U_HMAP_ERR_NONE     on success
 * \retval  U_HMAP_ERR_FAIL     on failure
 */
int u_hmap_get (u_hmap_t *hmap, const void *key, u_hmap_o_t **obj)
{
    dbg_err_if (hmap == NULL);
    dbg_err_if (key == NULL);
    dbg_err_if (obj == NULL);

    if (__get(hmap, key, obj))
    {
        *obj = NULL;
        return U_HMAP_ERR_FAIL;
    }
    dbg_err_if (obj == NULL);

    if (hmap->pcy.ops & U_HMAP_PCY_OP_GET)
        dbg_err_if (hmap->pcy.push(hmap, *obj, &(*obj)->pqe));

    return U_HMAP_ERR_NONE;

err:
    return U_HMAP_ERR_FAIL;
}

/**
 * \brief   Delete an object from the hmap
 *
 * Delete object with given \p key from \p hmap and return it (if the object is
 * owned by user).
 *
 * \param   hmap      hmap object
 * \param   key       key of object to be deleted
 * \param   obj       deleted object
 *
 * \retval  U_HMAP_ERR_NONE     on success
 * \retval  U_HMAP_ERR_FAIL     on failure
 */
int u_hmap_del (u_hmap_t *hmap, const void *key, u_hmap_o_t **obj)
{
    u_hmap_o_t *o = NULL;

    dbg_err_if (hmap == NULL);
    dbg_err_if (key == NULL);

    if (obj)
        *obj = NULL;

    if (__get(hmap, key, &o))
        return U_HMAP_ERR_FAIL;

    dbg_err_if (o == NULL);
    LIST_REMOVE(o, next);

    if (hmap->opts->options & U_HMAP_OPTS_OWNSDATA)
        __o_free(hmap, o);
    else
        if (obj)
            *obj = o;

    hmap->sz--;

    return U_HMAP_ERR_NONE;

err:
    return U_HMAP_ERR_FAIL;
}

/**
 * \brief   Create a data object (<b>unused for hmap_easy interface</b>)
 *
 * Creates a new (key, value) tuple to be inserted into a hmap. By default, the
 * user is responsible for allocation and deallocation of these objects and
 * their content. If the option U_HMAP_OPTS_OWNSDATA is set
 *
 * \param   hmap      reference to parent hmap
 * \param   key       pointer to the key
 * \param   val       pointer to the oject
 *
 * \return pointer to a new u_hmap_o_t
 */
u_hmap_o_t *u_hmap_o_new (u_hmap_t *hmap, const void *key, const void *val)
{
    u_hmap_o_t *obj = NULL;

    dbg_return_if (hmap == NULL, NULL);
    dbg_return_if (key == NULL, NULL);
    dbg_return_if (val == NULL, NULL);

    dbg_err_sif ((obj = (u_hmap_o_t *) u_zalloc(sizeof(u_hmap_o_t))) == NULL);
    obj->hmap = hmap;

    if (hmap->opts->options & U_HMAP_OPTS_OWNSDATA)
    {
        /* internalise key */
        switch (hmap->opts->key_type)
        {
            case U_HMAP_OPTS_DATATYPE_POINTER:
                memcpy(&obj->key, &key, sizeof(void **));
                break;
            case U_HMAP_OPTS_DATATYPE_STRING:
                dbg_err_if ((obj->key = u_strdup((const char *) key)) == NULL);
                break;
            case U_HMAP_OPTS_DATATYPE_OPAQUE:
                dbg_err_if ((obj->key = u_zalloc(hmap->opts->key_sz)) == NULL);
                memcpy(obj->key, key, hmap->opts->key_sz);
                break;
        }

        /* internalise value */
        switch (hmap->opts->val_type)
        {
            case U_HMAP_OPTS_DATATYPE_POINTER:
                memcpy(&obj->val, &val, sizeof(void **));
                break;
            case U_HMAP_OPTS_DATATYPE_STRING:
                dbg_err_if ((obj->val = u_strdup((const char *) val)) == NULL);
                break;
            case U_HMAP_OPTS_DATATYPE_OPAQUE:
                dbg_err_if ((obj->val = u_zalloc(hmap->opts->val_sz)) == NULL);
                memcpy(obj->val, val, hmap->opts->val_sz);
                break;
        }
    }
    else
    {  /* data owned by user - do not internalise, just set pointers */
        memcpy(&obj->key, &key, sizeof(void **));
        memcpy(&obj->val, &val, sizeof(void **));
    }

    obj->pqe = NULL;

    return obj;

err:
    U_FREEF(obj, u_hmap_o_free);

    return NULL;
}

/**
 * \brief  Free a data object (<b>unused for hmap_easy interface</b>)
 *
 * Frees a data object (without freeing its content). This function should only
 * be used if U_HMAP_OPTS_OWNSDATA is not set to free objects allocated with
 * u_hmap_o_new(). If U_HMAP_OPTS_OWNSDATA is set, the data is freed
 * automatically by the hashmap by using the default free function or the
 * overridden f_free().
 *
 * \param   obj       hmap object
 */
void u_hmap_o_free (u_hmap_o_t *obj)
{
    dbg_ifb (obj == NULL) return;

    u_free(obj);
}

/** \brief  Access the key of the hmap element pointed to by \p obj. */
void *u_hmap_o_get_key (u_hmap_o_t *obj)
{
    return obj->key;
}

/** \brief  Access the value of the hmap element pointed to by \p obj. */
void *u_hmap_o_get_val (u_hmap_o_t *obj)
{
    return obj->val;
}

/* Free a data object including content (only if U_HMAP_OPTS_OWNSDATA) */
static void __o_free (u_hmap_t *hmap, u_hmap_o_t *obj)
{
    dbg_ifb (hmap == NULL) return;
    dbg_ifb (obj == NULL) return;

    if (hmap->opts->f_free)
    {
        hmap->opts->f_free(obj);
    }
    else
    {
        switch (hmap->opts->key_type)
        {
            case U_HMAP_OPTS_DATATYPE_POINTER:
                if (hmap->opts->f_key_free)
                    hmap->opts->f_key_free(obj->key);
                break;
            case U_HMAP_OPTS_DATATYPE_STRING:
            case U_HMAP_OPTS_DATATYPE_OPAQUE:
                u_free(obj->key);
                break;
        }
        switch (hmap->opts->val_type)
        {
            case U_HMAP_OPTS_DATATYPE_POINTER:
                if (hmap->opts->f_val_free)
                    hmap->opts->f_val_free(obj->val);
                break;
            case U_HMAP_OPTS_DATATYPE_STRING:
            case U_HMAP_OPTS_DATATYPE_OPAQUE:
                u_free(obj->val);
                break;
        }
    }

    u_hmap_o_free(obj);
}

static int __o_overwrite (u_hmap_t *hmap, u_hmap_o_t *o, u_hmap_o_t *obj,
        u_hmap_o_t **old)
{
    /* overwrite */
    if (!(hmap->opts->options &
                U_HMAP_OPTS_NO_OVERWRITE))
    {
        if (hmap->pcy.ops & U_HMAP_PCY_OP_PUT)
        {
            /* delete old object and enqueue new one */
            dbg_err_if (__queue_del(hmap, o));
            dbg_err_if (hmap->pcy.push(hmap, obj, &obj->pqe));
        }

        /* replace object in hmap list */
        LIST_INSERT_AFTER(o, obj, next);
        LIST_REMOVE(o, next);

        if (hmap->opts->options & U_HMAP_OPTS_OWNSDATA)
            __o_free(hmap, o);
        else
            if (old)
                *old = o;

        return U_HMAP_ERR_NONE;
    }
    else  /* don't overwrite */
    {
        if (hmap->opts->options & U_HMAP_OPTS_OWNSDATA)
            __o_free(hmap, obj);
        else
            if (old)
                *old = o;

        return U_HMAP_ERR_EXISTS;
    }
err:
    return U_HMAP_ERR_FAIL;
}

/**
 *  \brief  Count the number of elements currently stored in hmap
 *
 *  Count the number of elements currently stored in the supplied \p hmap
 *  object
 *
 *  \param  hmap    hmap object
 *
 *  \return The number of elements currently stored in hmap, or -1 on error
 */
ssize_t u_hmap_count (u_hmap_t *hmap)
{
    dbg_return_if (hmap == NULL, -1);
    return hmap->sz;
}

/**
 *      \}
 */

/**
 *      \defgroup hmap_opts Options
 *      \{
 */

/**
 * \brief   Create new hmap options
 *
 * Create a new hmap options object and save its pointer to \p *opts.
 * The fields within the object can be manipulated publicly according to the
 * description in the header file.
 *
 * \param   opts      on success contains the hmap options object
 *
 * \retval  U_HMAP_ERR_NONE     on success
 * \retval  U_HMAP_ERR_FAIL     on failure
 */
int u_hmap_opts_new (u_hmap_opts_t **opts)
{
    u_hmap_opts_t *o;

    dbg_err_if (opts == NULL);

    dbg_err_sif ((o = (u_hmap_opts_t *) u_zalloc(sizeof(u_hmap_opts_t)))
            == NULL);

    u_hmap_opts_init(o);

    *opts = o;

    return U_HMAP_ERR_NONE;
err:
    *opts = NULL;
    return U_HMAP_ERR_FAIL;
}

/**
 * \brief   Deallocate hmap options
 *
 * Deallocate hmap options object \p opts.
 *
 * \param   opts      hmap options
 */
void u_hmap_opts_free (u_hmap_opts_t *opts)
{
    dbg_ifb (opts == NULL) return;

    u_free(opts);
}

/**
 * \brief   Initialise hmap options
 *
 * Set allocated hmap options to default values
 *
 * \param   opts      hmap options object
 */
void u_hmap_opts_init (u_hmap_opts_t *opts)
{
    dbg_ifb (opts == NULL) return;

    opts->size = U_HMAP_MAX_SIZE;
    opts->type = U_HMAP_TYPE_CHAIN;
    opts->max = U_HMAP_MAX_ELEMS;
    opts->policy = U_HMAP_PCY_NONE;
    opts->options = U_HMAP_OPTS_NO_OVERWRITE | U_HMAP_OPTS_OWNSDATA;
    opts->f_hash = &__f_hash;
    opts->f_comp = &__f_comp;
    opts->f_free = NULL;
    opts->key_type = U_HMAP_OPTS_DATATYPE_STRING;
    opts->val_type = U_HMAP_OPTS_DATATYPE_POINTER;
    opts->f_key_free = NULL;
    opts->f_val_free = NULL;
    opts->val_free_set = 0;
    opts->f_str = NULL;
    opts->key_sz = sizeof(void *);
    opts->val_sz = sizeof(void *);
    opts->easy = 0;

    return;
}

/**
 * \brief Copy options
 *
 * Perform a deep copy of options object \p from to \p to.
 *
 * \param   to        destination options
 * \param   from      source options
 *
 * \retval  U_HMAP_ERR_NONE     on success
 * \retval  U_HMAP_ERR_FAIL     on failure
 */
int u_hmap_opts_copy (u_hmap_opts_t *to, u_hmap_opts_t *from)
{
    dbg_err_if (to == NULL);
    dbg_err_if (from == NULL);

    memcpy(to, from, sizeof(u_hmap_opts_t));

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/**
 * \brief Debug options
 *
 * Print out information on option settings.
 *
 * \param   opts  options object
 */
void u_hmap_opts_dbg (u_hmap_opts_t *opts)
{
    dbg_return_if (opts == NULL, );

    u_dbg("<hmap_options>");
    u_dbg("  easy: %s", opts->easy ? "yes" : "no");
    u_dbg("  discard policy: %s", __pcy2str(opts->policy));
    u_dbg("  buckets: %u (max: %u -- if discard policy != none)",
            opts->size, opts->max);
    u_dbg("  owns data: %s",
            (opts->options & U_HMAP_OPTS_OWNSDATA) ? "yes" : "no");
    u_dbg("  objects dtor: %s", opts->f_free ? "set" : "not set");
    u_dbg("  keys dtor: %s", opts->f_key_free ? "set" : "not set");
    u_dbg("  values dtor: %s", opts->f_val_free ? "set" : "not set");
    u_dbg("  object serializer: %s", opts->f_str ? "set" : "not set");
    u_dbg("  overwrite equal keys: %s",
            (opts->options & U_HMAP_OPTS_NO_OVERWRITE) ? "no" : "yes");
    u_dbg("  key type: %s", __datatype2str(opts->key_type));
    u_dbg("  value type: %s", __datatype2str(opts->val_type));
    u_dbg("</hmap_options>");

    return;
}

/** \brief Set initial size of hmap's dynamic array */
int u_hmap_opts_set_size (u_hmap_opts_t *opts, int sz)
{
    dbg_err_if (opts == NULL || sz <= 0);

    opts->size = sz;

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/** \brief Set maximum number of elements
 *
 * This limit is used only if a discard policy has been set via
 * u_hmap_opts_set_policy(); otherwise the hmap is simply resized.
 */
int u_hmap_opts_set_max (u_hmap_opts_t *opts, int max)
{
    dbg_err_if (opts == NULL || max <= 0);

    opts->max = max;

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/** \brief Set type of hmap */
int u_hmap_opts_set_type (u_hmap_opts_t *opts, u_hmap_type_t type)
{
    dbg_err_if (opts == NULL);
    dbg_err_if (!U_HMAP_IS_TYPE(type));

    opts->type = type;

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/** \brief Set option in options mask
  (<b>hmap_easy interface cannot operate on U_HMAP_OPTS_OWNSDATA</b>) */
int u_hmap_opts_set_option (u_hmap_opts_t *opts, int option)
{
    dbg_err_if (opts == NULL);

    dbg_err_if ((option != U_HMAP_OPTS_OWNSDATA &&
            option != U_HMAP_OPTS_NO_OVERWRITE &&
            option != U_HMAP_OPTS_HASH_STRONG));

    dbg_err_if (opts->easy &&
            option == U_HMAP_OPTS_OWNSDATA);

    opts->options |= option;

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/** \brief Unset option in options mask
  (<b>hmap_easy interface cannot operate on U_HMAP_OPTS_OWNSDATA</b>) */
int u_hmap_opts_unset_option (u_hmap_opts_t *opts, int option)
{
    dbg_err_if (opts == NULL);

    dbg_err_if ((option != U_HMAP_OPTS_OWNSDATA &&
            option != U_HMAP_OPTS_NO_OVERWRITE &&
            option != U_HMAP_OPTS_HASH_STRONG));

    dbg_err_if (opts->easy &&
            option == U_HMAP_OPTS_OWNSDATA);

    opts->options &= ~option;

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/** \brief Set a custom hash function */
int u_hmap_opts_set_hashfunc (u_hmap_opts_t *opts,
        size_t (*f_hash)(const void *key, size_t buckets))
{
    dbg_err_if (opts == NULL || f_hash == NULL);

    opts->f_hash = f_hash;

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/** \brief Set a custom key comparison function */
int u_hmap_opts_set_compfunc (u_hmap_opts_t *opts,
        int (*f_comp)(const void *k1, const void *k2))
{
    dbg_err_if (opts == NULL || f_comp == NULL);

    opts->f_comp = f_comp;

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/** \brief Set a custom object free function
  (<b>not available for hmap_easy interface</b>) */
int u_hmap_opts_set_freefunc (u_hmap_opts_t *opts,
        void (*f_free)(u_hmap_o_t *obj))
{
    dbg_err_if (opts == NULL || opts->easy);

    opts->f_free = f_free;

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/** \brief Set a custom string representation for objects */
int u_hmap_opts_set_strfunc (u_hmap_opts_t *opts,
        u_string_t *(*f_str)(u_hmap_o_t *obj))
{
    dbg_err_if (opts == NULL);

    opts->f_str = f_str;

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/** \brief Set type for keys
  (<b>not available for hmap_easy interface</b>) */
int u_hmap_opts_set_key_type (u_hmap_opts_t *opts,
        u_hmap_options_datatype_t type)
{
    dbg_err_if (opts == NULL);
    dbg_err_if (!U_HMAP_IS_DATATYPE(type));

    /* not available for hmap_easy interface */
    dbg_err_ifm (opts->easy,
            "cannot set data type when \"easy\" interface is in use");

    opts->key_type = type;

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/** \brief Set size of keys (<b>not available for hmap_easy interface</b>,
  * <b>only valid for U_HMAP_OPTS_DATATYPE_OPAQUE key type</b>) */
int u_hmap_opts_set_key_sz (u_hmap_opts_t *opts, size_t sz)
{
    dbg_err_if (opts == NULL || sz == 0);

    dbg_err_if (opts->easy);
    dbg_err_if (opts->key_type != U_HMAP_OPTS_DATATYPE_OPAQUE);

    opts->key_sz = sz;

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/** \brief Set free function for keys
(<b>not available for hmap_easy interface</b>) */
int u_hmap_opts_set_key_freefunc (u_hmap_opts_t *opts,
        void (*f_free)(const void *key))
{
    dbg_err_if (opts == NULL);

    dbg_err_if (opts->easy);

    opts->f_key_free = f_free;

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/** \brief Set type of values */
int u_hmap_opts_set_val_type (u_hmap_opts_t *opts,
        u_hmap_options_datatype_t type)
{
    dbg_err_if (opts == NULL);
    dbg_err_if (!U_HMAP_IS_DATATYPE(type));

    opts->val_type = type;

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/** \brief Set size of values
  (<b>only valid for U_HMAP_OPTS_DATATYPE_OPAQUE key type</b>) */
int u_hmap_opts_set_val_sz (u_hmap_opts_t *opts, size_t sz)
{
    dbg_err_if (opts == NULL || sz == 0);

    dbg_err_if (opts->val_type != U_HMAP_OPTS_DATATYPE_OPAQUE);

    opts->val_sz = sz;

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/** \brief Set free function for values */
int u_hmap_opts_set_val_freefunc (u_hmap_opts_t *opts,
        void (*f_free)(void *val))
{
    dbg_err_if (opts == NULL);

    opts->f_val_free = f_free;
    opts->val_free_set = 1;     /* keep track of this because setting it is
                                   FORCED when using easy interface */
    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/**
 *      \}
 */

/**
 *      \defgroup hmap_pcy Policy Settings
 *      \{
 */

/** \brief Set discard policy */
int u_hmap_opts_set_policy (u_hmap_opts_t *opts, u_hmap_pcy_type_t policy)
{
    dbg_err_if (opts == NULL);
    dbg_err_if (!U_HMAP_IS_PCY(policy));

    opts->policy = policy;

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/** \brief Set custom policy comparison function
 *
 * Sets the object comparison function for U_HMAP_PCY_CUSTOM discard policy.
 * The function should return 1 if \p o1 has higher priority than \p o2, 0 if
 * it is equal and -1 if the priority is lower.
 */
int u_hmap_opts_set_policy_cmp (u_hmap_opts_t *opts,
        int (*f_pcy_cmp)(void *o1, void *o2))
{
    dbg_err_if (opts == NULL || f_pcy_cmp == NULL);
    dbg_err_if (opts->policy != U_HMAP_PCY_CUSTOM);

    opts->f_pcy_cmp = f_pcy_cmp;

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/**
 *      \}
 */

/**
 *      \defgroup hmap_common Other Utilities
 *      \{
 */

/**
 * \brief Get a string representation of an error code
 *
 * Get a string representation of an error code
 *
 * \param   rc   return code
 */
const char *u_hmap_strerror (u_hmap_ret_t rc)
{
    switch (rc)
    {
        case U_HMAP_ERR_NONE:
            return "success";
        case U_HMAP_ERR_FAIL:
            return "general failure";
        case U_HMAP_ERR_EXISTS:
            return "element already exists in table";
    }
    return NULL;
}

/**
 * \brief Copy hmap
 *
 * Perform a copy of an hmap \p from to \p to by rehashing all elements and
 * copying the object pointers to the new locations.
 *
 * \param   to        destination hmap
 * \param   from      source hmap
 *
 * \retval  U_HMAP_ERR_NONE     on success
 * \retval  U_HMAP_ERR_FAIL     on failure
 */
int u_hmap_copy (u_hmap_t *to, u_hmap_t *from)
{
    u_hmap_o_t *obj;
    size_t i;

    dbg_err_if (to == NULL);
    dbg_err_if (from == NULL);

    for (i = 0; i < from->size; ++i)
    {
        while ((obj = LIST_FIRST(&from->hmap[i])) != NULL)
        {
            LIST_REMOVE(obj, next);
            dbg_err_if (u_hmap_put(to, obj, NULL));
        }
    }

    return U_HMAP_ERR_NONE;

err:
    return U_HMAP_ERR_FAIL;
}

/* Retrieve an hmap element given a key */
static int __get (u_hmap_t *hmap, const void *key, u_hmap_o_t **o)
{
    u_hmap_o_t *obj;
    u_hmap_e_t *x;
    int comp;
    size_t hash;
    size_t last;

    dbg_err_if (hmap == NULL);
    dbg_err_if (key == NULL);
    dbg_err_if (o == NULL);

    hash = hmap->opts->f_hash(key, hmap->size);

    if (hmap->opts->f_hash != &__f_hash &&
            !(hmap->opts->options & U_HMAP_OPTS_HASH_STRONG))
    {
        enum { MAX_INT = 20 };
        char h[MAX_INT];

        u_snprintf(h, MAX_INT, "%u", hash);
        hash = __f_hash(h, hmap->size);
    }

    x = &hmap->hmap[hash];

    switch (hmap->opts->type)
    {
        case U_HMAP_TYPE_CHAIN:

            LIST_FOREACH(obj, x, next)
            {
                if ((comp = hmap->opts->f_comp(key, obj->key)) == 0)
                { /* object found */
                    *o = obj;
                    return U_HMAP_ERR_NONE;
                }
                else if (comp < 0)  /* cannot be in list (ordered) */
                {
                    *o = NULL;
                    break;
                }
            }
            break;

        case U_HMAP_TYPE_LINEAR:

            last = ((hash + hmap->size -1) % hmap->size);

            for (; hash != last; hash = ((hash +1) % hmap->size),
                    x = &hmap->hmap[hash])
            {
                if (!LIST_EMPTY(x))
                {
                    obj = LIST_FIRST(x);

                    if ((hmap->opts->f_comp(key, obj->key)) == 0)
                    {
                        *o = obj;
                        return U_HMAP_ERR_NONE;
                    }
                }
            }
            break;
    }

err:
    return U_HMAP_ERR_FAIL;
}

/* pop the front of an object queue */
static int __queue_pop_front (u_hmap_t *hmap, u_hmap_o_t **obj)
{
    u_hmap_q_t *first;

    dbg_err_if (hmap == NULL);

    dbg_err_if ((first = TAILQ_FIRST(&hmap->pcy.queue)) == NULL);
    dbg_err_if (u_hmap_del(hmap, first->ho->key, obj));
    TAILQ_REMOVE(&hmap->pcy.queue, first, next);
    __q_o_free(first);

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/* pop the back of an object queue */
static int __queue_pop_back (u_hmap_t *hmap, u_hmap_o_t **obj)
{
    u_hmap_q_t *last;

    dbg_err_if (hmap == NULL);

    dbg_err_if ((last = TAILQ_LAST(&hmap->pcy.queue, u_hmap_q_h_s))
            == NULL);
    dbg_err_if (u_hmap_del(hmap, last->ho->key, obj));
    TAILQ_REMOVE(&hmap->pcy.queue, last, next);
    __q_o_free(last);

    return U_HMAP_ERR_NONE;

err:
    return U_HMAP_ERR_FAIL;
}

/* push object data onto queue */
static int __queue_push (u_hmap_t *hmap, u_hmap_o_t *obj,
        u_hmap_q_t **data)
{
    u_hmap_q_t *new;

    dbg_err_if (hmap == NULL);
    dbg_err_if (obj == NULL);
    dbg_err_if (data == NULL);

    if (*data == NULL)
    {  /* no existing reference to queue entry -> push to head
          (FIFO will always enter here) */
        dbg_err_if ((new = __q_o_new(obj)) == NULL);
        TAILQ_INSERT_HEAD(&hmap->pcy.queue, new, next);
        *data = new;
    } else {
        /* we already have an element in the queue -> move to head
           (LRU will enter here for all ops after first insert) */
        TAILQ_REMOVE(&hmap->pcy.queue, *data, next);
        TAILQ_INSERT_HEAD(&hmap->pcy.queue, *data, next);
    }

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/* Increment count data object and push onto queue (LFU) */
static int __queue_push_count (u_hmap_t *hmap, u_hmap_o_t *obj,
        u_hmap_q_t **counts)
{
    u_hmap_q_t *new, *q;
    int *count;

    dbg_err_if (hmap == NULL);
    dbg_err_if (obj == NULL);
    dbg_err_if (counts == NULL);

    if (*counts == NULL)
    {
        /* no reference to queue entry -> count == 0 */
        dbg_err_if ((new = __q_o_new(obj)) == NULL);
        TAILQ_INSERT_HEAD(&hmap->pcy.queue, new, next);
        dbg_err_sif ((count = (int *) u_zalloc(sizeof(int))) == NULL);
        new->data = (void *) count;
        *counts = new;
        /*
        u_dbg(">k: %s, p: %d", new->ho->key, *((int *)new->data));
        */
    }
    else
    {
        /* we already have an element in the queue -> increment count
         and reposition it accordingly */
        count = (int *) (*counts)->data;
        *count = *count + 1;
        /*
        u_dbg("[");
        TAILQ_FOREACH(q, &hmap->pcy.queue, next)
            u_dbg("k: %s, p: %d", q->ho->key, *((int *)q->data));
        u_dbg("]");
        */
        for (q = TAILQ_NEXT(*counts, next);
                q && ((*count) >= *((int *) q->data));
                q = TAILQ_NEXT(q, next))
            ;
        TAILQ_REMOVE(&hmap->pcy.queue, *counts, next);
        if (q)  /* found an object with >= count -> insert just before */
            TAILQ_INSERT_BEFORE(q, *counts, next);
        else  /* we have the largest count -> last element */
            TAILQ_INSERT_TAIL(&hmap->pcy.queue, *counts, next);
    }

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/* Push elements onto priority queue according to custom comparison function */
static int __queue_push_cmp (u_hmap_t *hmap, u_hmap_o_t *obj,
        u_hmap_q_t **data)
{
    u_hmap_q_t *new, *q;

    dbg_err_if (hmap == NULL);
    dbg_err_if (obj == NULL);
    dbg_err_if (data == NULL);

    /* no internal policy object should exist since it is managed externally */
    dbg_err_if (*data != NULL);

    dbg_err_if ((new = __q_o_new(obj)) == NULL);

    if (TAILQ_EMPTY(&hmap->pcy.queue))
    {
       TAILQ_INSERT_HEAD(&hmap->pcy.queue, new, next);
    }
    else
    {
        TAILQ_FOREACH(q, &hmap->pcy.queue, next)
        {
            /* found an object with >= priority -> insert just before */
            if (hmap->opts->f_pcy_cmp(obj->val, q->ho->val) >= 0)
            {
                TAILQ_INSERT_BEFORE(q, new, next);
                break;
            }
        }
        if (q == NULL)  /* reached end - append */
            TAILQ_INSERT_TAIL(&hmap->pcy.queue, new, next);
    }

    *data = new;

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/* Delete a specific queue element */
static int __queue_del (u_hmap_t *hmap, u_hmap_o_t *obj)
{
    dbg_err_if (hmap == NULL);
    dbg_err_if (obj == NULL);

    TAILQ_REMOVE(&hmap->pcy.queue, obj->pqe, next);
    __q_o_free(obj->pqe);
    obj->pqe = NULL;

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/**
 * \brief   Perform an operation on all objects
 *
 * Execute function \p f on all objects within \p hmap. These functions should
 * return U_HMAP_ERR_NONE on success, and take an object as a parameter.
 *
 * \param   hmap      hmap object
 * \param   f         function
 *
 * \retval  U_HMAP_ERR_NONE     on success
 * \retval  U_HMAP_ERR_FAIL     on failure
 */
int u_hmap_foreach (u_hmap_t *hmap, int f(const void *val))
{
    u_hmap_o_t *obj;
    size_t i;

    dbg_err_if (hmap == NULL);
    dbg_err_if (f == NULL);

    for (i = 0; i < hmap->size; ++i)
    {
        LIST_FOREACH(obj, &hmap->hmap[i], next)
            dbg_err_if (f(obj->val));
    }

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/**
 * \brief   \c u_hmap_foreach with user supplied parameter
 *
 * Execute function \p f on all objects within \p hmap supplying \p arg
 * as custom argument.  These functions should return \c U_HMAP_ERR_NONE on
 * success, and take an hmap object and \p arg as parameters.
 *
 * \param   hmap      hmap object
 * \param   f         function
 * \param   arg       custom arg that will be supplied to \p f
 *
 * \retval  U_HMAP_ERR_NONE     on success
 * \retval  U_HMAP_ERR_FAIL     on failure
 */
int u_hmap_foreach_arg (u_hmap_t *hmap,
        int f(const void *val, const void *arg), void *arg)
{
    size_t i;
    u_hmap_o_t *obj;

    dbg_err_if (hmap == NULL);
    dbg_err_if (f == NULL);

    for (i = 0; i < hmap->size; ++i)
    {
        LIST_FOREACH(obj, &hmap->hmap[i], next)
            dbg_err_if (f(obj->val, arg));
    }

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/**
 * \brief   Perform an operation on all objects
 *
 * Execute function \p f on all objects within \p hmap. These functions should
 * return U_HMAP_ERR_NONE on success, and take an object as a parameter.
 *
 * \param   hmap      hmap object
 * \param   f         function, must accept key and val params
 *
 * \retval  U_HMAP_ERR_NONE     on success
 * \retval  U_HMAP_ERR_FAIL     on failure
 */
int u_hmap_foreach_keyval(u_hmap_t *hmap,
        int f(const void *key, const void *val))
{
    struct u_hmap_o_s *obj;
    size_t i;

    dbg_err_if (hmap == NULL);
    dbg_err_if (f == NULL);

    for (i = 0; i < hmap->size; ++i)
    {
        LIST_FOREACH(obj, &hmap->hmap[i], next)
            dbg_err_if (f(obj->key, obj->val));
    }

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

/**
 * \brief Debug Hmap
 *
 * Print out information on an hmap.
 *
 * \param   hmap  hmap object
 */
void u_hmap_dbg (u_hmap_t *hmap)
{
    enum { MAX_LINE = 255 };
    u_string_t *s = NULL, *st = NULL;
    u_hmap_o_t *obj;
    size_t i;

    dbg_ifb (hmap == NULL) return;

    for (i = 0; i < hmap->size; ++i)
    {
        if (LIST_FIRST(&hmap->hmap[i]) == NULL)
            continue;

        dbg_ifb (u_string_create("", 1, &s)) return;
        dbg_err_if (u_string_clear(s));
        dbg_err_if (u_string_aprintf(s, "%5d ", i));

        LIST_FOREACH(obj, &hmap->hmap[i], next)
        {
            if (hmap->opts->f_str == NULL)
            {
                dbg_err_if (u_string_cat(s, "[]"));
            }
            else
            {
                st = hmap->opts->f_str(obj);
                dbg_err_if (u_string_append(s, u_string_c(st),
                            u_string_len(st)-1));
                u_string_free(st);
            }
        }
        u_dbg(u_string_c(s));
        u_string_free(s);
    }

    return;
err:
    U_FREEF(st, u_string_free);
    U_FREEF(s, u_string_free);
    return;
}

/**
 * \brief Debug policy
 *
 * Print out policy information.
 *
 * \param hmap  hmap object
 */
void u_hmap_pcy_dbg (u_hmap_t *hmap)
{
    u_hmap_q_t *q;
    u_string_t *s = NULL, *s2 = NULL;

    dbg_ifb (hmap == NULL) return;

    dbg_ifb (u_string_create("", 1, &s)) return;
    dbg_err_if (u_string_clear(s));
    dbg_err_if (u_string_cat(s, "Policy: "));

    TAILQ_FOREACH(q, &hmap->pcy.queue, next)
    {
        s2 = hmap->opts->f_str(q->ho);
        dbg_err_if (u_string_cat(s, u_string_c(s2)));
        if (hmap->opts->policy == U_HMAP_PCY_LFU)  /* print count */
            dbg_err_if (u_string_aprintf(s, "-%d", *((int *)q->data)));
        u_string_free(s2);
    }

    u_dbg(u_string_c(s));
    u_string_free(s);

    return;
err:
    U_FREEF(s, u_string_free);
    U_FREEF(s2, u_string_free);
    return;
}

/* Allocate a new queue data object */
static u_hmap_q_t *__q_o_new (u_hmap_o_t *ho)
{
    u_hmap_q_t *qo = NULL;

    dbg_return_if (ho == NULL, NULL);

    dbg_err_sif ((qo = (u_hmap_q_t *)
                u_zalloc(sizeof(u_hmap_q_t))) == NULL);

    qo->ho = ho;
    qo->data = NULL;

    return qo;
err:
    u_free(qo);
    return NULL;
}

/* Free a data queue object */
static void __q_o_free (u_hmap_q_t *qo)
{
    dbg_ifb (qo == NULL) return;

    if (qo->data)
        u_free(qo->data);
    u_free(qo);
}

/* Get a string representation of a policy */
static const char *__pcy2str (u_hmap_pcy_type_t policy)
{
    switch (policy)
    {
        case U_HMAP_PCY_NONE:
            return "none";
        case U_HMAP_PCY_FIFO:
            return "fifo";
        case U_HMAP_PCY_LRU:
            return "lru";
        case U_HMAP_PCY_LFU:
            return "lfu";
        case U_HMAP_PCY_CUSTOM:
            return "custom";
    }
    return NULL;
}

/* Get a string representation of a policy */
static const char *__datatype2str(u_hmap_options_datatype_t datatype)
{
    switch (datatype)
    {
        case U_HMAP_OPTS_DATATYPE_POINTER:
            return "pointer";
        case U_HMAP_OPTS_DATATYPE_STRING:
            return "string";
        case U_HMAP_OPTS_DATATYPE_OPAQUE:
            return "opaque";
    }

    return NULL;
}

static int __next_prime(size_t *prime, size_t sz, size_t *idx)
{
    static size_t primes[] = {
        13, 19, 29, 41, 59, 79, 107, 149, 197, 263, 347, 457, 599, 787, 1031,
        1361, 1777, 2333, 3037, 3967, 5167, 6719, 8737, 11369, 14783, 19219,
        24989, 32491, 42257, 54941, 71429, 92861, 120721, 156941, 204047,
        265271, 344857, 448321, 582821, 757693, 985003, 1280519, 1664681,
        2164111, 2813353, 3657361, 4754591, 6180989, 8035301, 10445899,
        13579681, 17653589, 22949669, 29834603, 38784989, 50420551, 65546729,
        85210757, 110774011, 144006217, 187208107, 243370577, 316381771,
        411296309, 534685237, 695090819, 903618083, 1174703521, 1527114613,
        1837299131, 2147483647
    };

    size_t i;

    dbg_err_if (prime == NULL);
    dbg_err_if (sz == 0);
    dbg_err_if (idx == NULL);

    for (i = *idx; i < sizeof(primes)/sizeof(size_t); ++i)
    {
        if (primes[i] >= sz)
        {
            *idx = i;
            *prime = primes[i];
            goto ok;
        }
    }
    dbg_err_ifm (1, "hmap size limit exceeded");

ok:
    return 0;

err:
    return ~0;
}

static int __resize(u_hmap_t *hmap)
{
    u_hmap_opts_t *newopts = NULL;
    u_hmap_t *newmap = NULL;

    dbg_err_if (hmap == NULL);

    if (hmap->opts->policy != U_HMAP_PCY_NONE)
        return 0;

    u_dbg("resize from: %u", hmap->size);

    /* copy old options */
    dbg_err_if (u_hmap_opts_new(&newopts));
    dbg_err_if (u_hmap_opts_copy(newopts, hmap->opts));

    /* set rate and create the new hashmap */
    dbg_err_if (__next_prime(&newopts->size,
            U_HMAP_RATE_RESIZE * newopts->size,
            &hmap->px));
    dbg_err_if (u_hmap_new(newopts, &newmap));
    u_hmap_opts_free(newopts);

    /* remove any ownership from old map to copy elements */
    hmap->opts->options &= !U_HMAP_OPTS_OWNSDATA;

    /* copy old elements to new map policy */
    dbg_err_if (u_hmap_copy(newmap, hmap));

    /* free old allocated objects */
    u_hmap_opts_free(hmap->opts);
    u_free(hmap->hmap);

    /* copy new map to this hmap */
    memcpy(hmap, newmap, sizeof(u_hmap_t));
    u_free(newmap);

    u_dbg("resized to: %u", hmap->size);

    return 0;
err:
    return ~0;
}

/**
 *      \}
 */

/**
 *   \}
 */
