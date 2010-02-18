/* 
 * Copyright (c) 2005-2010 by KoanLogic s.r.l. - All rights reserved.  
 */
/* solo oggetto in freefunc */
/* string interface */
/* overwrite? */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <toolbox/memory.h>
#include <toolbox/carpal.h>
#include <toolbox/hmap.h>
#include <toolbox/misc.h>

/**
 *  \defgroup hmap Map 
 *  \{
 */

/* default limits handled by policies */
#define U_HMAP_MAX_SIZE      512
#define U_HMAP_MAX_ELEMS     U_HMAP_MAX_SIZE
#define U_HMAP_RATE_FULL     0.75
#define U_HMAP_RATE_RESIZE   3

enum {
    U_HMAP_PCY_OP_PUT = 0x1,
    U_HMAP_PCY_OP_GET = 0x2
};

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

static int __get (u_hmap_t *hmap, const void *key, 
        u_hmap_o_t **o);

static int __opts_check (u_hmap_opts_t *opts);
static int __pcy_setup (u_hmap_t *hmap);
static const char *__pcy2str(u_hmap_pcy_type_t policy);

static void __o_free (u_hmap_t *hmap, u_hmap_o_t *obj);

static u_hmap_q_t *__q_o_new (const void *key);
static void __q_o_free (u_hmap_q_t *s);

static size_t __f_hash (const void *key, size_t size);
static int __f_comp (const void *k1, const void *k2);
static void __f_free (u_hmap_o_t *obj);
static u_string_t *__f_str (u_hmap_o_t *obj);

static int __queue_push (u_hmap_t *hmap, u_hmap_o_t *obj, 
        u_hmap_q_t **data);
static int __queue_push_count (u_hmap_t *hmap, u_hmap_o_t *obj,
        u_hmap_q_t **counts);
static int __queue_pop_front (u_hmap_t *hmap, u_hmap_o_t **obj);
static int __queue_pop_back (u_hmap_t *hmap, u_hmap_o_t **obj);

static int __resize(u_hmap_t *hmap);
static int __next_prime(size_t *prime, size_t sz, size_t *idx);

int u_hmap_easy_new (u_hmap_opts_t *opts, u_hmap_t **phmap)
{
    u_hmap_opts_t newopts;

    dbg_err_if (phmap == NULL);

    u_hmap_opts_init(&newopts);

    if (opts) {
        dbg_err_if (u_hmap_opts_copy(&newopts, opts));
        if (newopts.options == 0) {
            newopts.options |= U_HMAP_OPTS_NO_OVERWRITE;
            newopts.options |= U_HMAP_OPTS_OWNSDATA;
        }
    } else {
        newopts.options |= U_HMAP_OPTS_NO_OVERWRITE;
        newopts.options |= U_HMAP_OPTS_OWNSDATA;
    }
    newopts.easy = 1;
    newopts.f_free = NULL;

    dbg_err_if (u_hmap_new(&newopts, phmap));

    return U_HMAP_ERR_NONE;
err:
    return U_HMAP_ERR_FAIL;
}

void u_hmap_easy_free (u_hmap_t *hmap)
{
    dbg_err_if (hmap == NULL);

    u_hmap_free(hmap);
err:
    return; 
}

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

void *u_hmap_easy_get (u_hmap_t *hmap, const char *key)
{
    u_hmap_o_t *obj = NULL;

    dbg_err_if (u_hmap_get(hmap, key, &obj));

    return obj->val;
err:
    return NULL;
}

int u_hmap_easy_del (u_hmap_t *hmap, const char *key)
{
    return (u_hmap_del(hmap, key, NULL));
}

/**
 * \brief Get a string representation of an error code
 *
 * Get a string representation of an error code
 *
 * \param rc   return code 
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

/* Default hash function */
static size_t __f_hash (const void *key, size_t size)
{
    size_t h = 0;
    unsigned char *k = (unsigned char *) key;

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

    return strcmp((char *)k1, (char *)k2);
}

/* Default function for freeing hmap key and value */
static void __f_free (u_hmap_o_t *obj)
{
    dbg_ifb (obj == NULL) return;

    u_free(obj->key); 
    u_free(obj->val); 
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
    dbg_err_if (opts->policy < U_HMAP_PCY_NONE || 
            opts->policy > U_HMAP_PCY_LFU);
    dbg_err_if (opts->f_hash == NULL);
    dbg_err_if (opts->f_comp == NULL);

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
        case U_HMAP_PCY_LRU:
            hmap->pcy.push = __queue_push;
            hmap->pcy.pop = __queue_pop_back;
            hmap->pcy.ops = U_HMAP_PCY_OP_PUT | U_HMAP_PCY_OP_GET;
            break;
        case U_HMAP_PCY_FIFO:
            hmap->pcy.push = __queue_push;
            hmap->pcy.pop = __queue_pop_back;
            hmap->pcy.ops = U_HMAP_PCY_OP_PUT;
            break;
        case U_HMAP_PCY_LFU:
            hmap->pcy.push = __queue_push_count;
            hmap->pcy.pop = __queue_pop_front;
            hmap->pcy.ops = U_HMAP_PCY_OP_PUT | U_HMAP_PCY_OP_GET;
            break;
        default:
            u_dbg("Invalid policy: %d", hmap->opts->policy);
            return U_HMAP_ERR_FAIL;
    }

    return U_HMAP_ERR_NONE;
}

/**
 * \brief   Create a new hmap 
 * 
 * Create a new hmap object and save its pointer to \a *hmap. 
 * The call may fail on memory allocation problems or if the options are
 * manipulated incorrectly.
 * 
 * \param opts      options to be passed to the hmap
 * \param phmap      on success contains the hmap options object
 * 
 * \return U_HMAP_ERR_NONE on success, U_HMAP_ERR_FAIL on failure
 */
int u_hmap_new (u_hmap_opts_t *opts, u_hmap_t **phmap)
{
    size_t i;
    u_hmap_t *c = NULL;

    /* allow (opts == NULL) */
    dbg_return_if (phmap == NULL, ~0);
   
    dbg_return_sif ((c = (u_hmap_t *) u_zalloc(sizeof(u_hmap_t))) == NULL, ~0);
    
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
 * \brief Copy hmap
 *
 * Perform a copy of an hmap \a from to \a to by rehashing all elements and
 * copying the object pointers to the new locations.
 *
 * \param to        destination hmap
 * \param from      source hmap
 * 
 * \return U_HMAP_ERR_NONE on success, U_HMAP_ERR_FAIL on failure
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

/**
 * \brief Debug Hmap
 *
 * Print out information on an hmap.
 *
 * \param hmap  hmap object
 */
void u_hmap_dbg (u_hmap_t *hmap)
{
    enum { MAX_LINE = 255 };
    u_string_t *s = NULL, *st = NULL;
    u_hmap_o_t *obj;
    size_t i;

    dbg_ifb (hmap == NULL) return;

    u_dbg ("<hmap>");
    for (i = 0; i < hmap->size; ++i) 
    {
        dbg_ifb (u_string_create("", 1, &s)) return;
        dbg_err_if (u_string_clear(s));
        dbg_err_if (u_string_append(s, "|", 1));

        LIST_FOREACH(obj, &hmap->hmap[i], next) 
        {
            if (hmap->opts->f_str == NULL) 
            {
                dbg_err_if (u_string_append(s, "[]", 2));
            } else {
                st = hmap->opts->f_str(obj);
                dbg_err_if (u_string_append(s, u_string_c(st),
                            u_string_len(st)-1));
                u_string_free(st);
            }
        } 
        dbg_err_if (u_string_append(s, "|", 1));
        u_dbg(u_string_c(s));
        dbg_ifb (u_string_free(s)) return;
    }
    u_dbg("</hmap>");
    return;

err:
    U_FREEF(st, u_string_free);
    U_FREEF(s, u_string_free);
    return;   
}

/*
 * \brief   Delete an object from the hmap
 * 
 * Delete object with given \a key from \a hmap and return it (if the object is
 * owned by user).
 * 
 * \param hmap      hmap object
 * \param key       key of object to be deleted 
 * \param obj       deleted object
 * 
 * \return U_HMAP_ERR_NONE on success, U_HMAP_ERR_FAIL on failure
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

/* Retrieve an hmap element given a key */
static int __get (u_hmap_t *hmap, const void *key, u_hmap_o_t **o)
{
    u_hmap_o_t *obj;
    u_hmap_e_t *x;
    int comp;
	size_t hash;

    dbg_err_if (hmap == NULL);
    dbg_err_if (key == NULL);
    dbg_err_if (o == NULL);

	hash = hmap->opts->f_hash(key, hmap->size);

	if (hmap->opts->f_hash != &__f_hash && 
			!(hmap->opts->options & U_HMAP_OPTS_HASH_STRONG)) {
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
                } else if (comp < 0) { /* cannot be in list (ordered) */
                    *o = NULL;
                    break;
                }
            }
            break;


        case U_HMAP_TYPE_LINEAR:
            {
                size_t last = ((hash + hmap->size -1) % hmap->size);

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
            }

            break;
    }

err: 
    return U_HMAP_ERR_FAIL;
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
    u_hmap_q_t *data;
    u_string_t *s = NULL;

    dbg_ifb (hmap == NULL) return;

    dbg_ifb (u_string_create("", 1, &s)) return;
    dbg_err_if (u_string_clear(s));
    dbg_err_if (u_string_append(s, "Policy: [", strlen("Policy: [")));

    TAILQ_FOREACH(data, &hmap->pcy.queue, next) 
    {
        dbg_err_if (u_string_append(s, "(", 1));
        dbg_err_if (u_string_append(s, data->key, strlen(data->key)));
        dbg_err_if (u_string_append(s, ")", 1));
    }
    dbg_err_if (u_string_append(s, "]", 1));
    u_dbg(u_string_c(s));
    dbg_if (u_string_free(s));

    return;
    
 err:
    U_FREEF(s, u_string_free);
    return;
}

/* pop the front of an object queue */
static int __queue_pop_front (u_hmap_t *hmap, u_hmap_o_t **obj)
{
    u_hmap_q_t *first;

    dbg_err_if (hmap == NULL);

    dbg_err_if ((first = TAILQ_FIRST(&hmap->pcy.queue)) == NULL);
    dbg_err_if (u_hmap_del(hmap, first->key, obj));
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
    dbg_err_if (u_hmap_del(hmap, last->key, obj));
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
    {  /* no reference to queue entry */
        dbg_err_if ((new = __q_o_new(obj->key)) == NULL);
        TAILQ_INSERT_HEAD(&hmap->pcy.queue, new, next);
        *data = new;
    } else { /* have element in queue - move to head */
        TAILQ_REMOVE(&hmap->pcy.queue, *data, next);
        TAILQ_INSERT_HEAD(&hmap->pcy.queue, *data, next);
    }
    return U_HMAP_ERR_NONE;
    
err:
    return U_HMAP_ERR_FAIL;
}

/* Increment count data object and push onto queue */
static int __queue_push_count (u_hmap_t *hmap, u_hmap_o_t *obj, 
        u_hmap_q_t **counts)
{
    u_hmap_q_t *new, *t;
    int *count;

    dbg_err_if (hmap == NULL);
    dbg_err_if (obj == NULL);
    dbg_err_if (counts == NULL);

    if (*counts == NULL) /* no reference to queue entry */
    {  
        dbg_err_if ((new = __q_o_new(obj->key)) == NULL);
        TAILQ_INSERT_HEAD(&hmap->pcy.queue, new, next);
        *counts = TAILQ_FIRST(&hmap->pcy.queue);
        dbg_err_sif ((count = (int *) u_zalloc(sizeof(int))) == NULL);
        new->o = (void *) count;
        *counts = new;
    } else { /* have element in queue - move to head */
        count = (int *) (*counts)->o;
        memset((void *) count, (*count)++, sizeof(int));

        if ((t = TAILQ_NEXT(*counts, next))) 
        {
            for (; t && ((*count) > *((int *) t->o)); t = TAILQ_NEXT(t, next))
                ;
            TAILQ_REMOVE(&hmap->pcy.queue, *counts, next);
            if (t)
                TAILQ_INSERT_BEFORE(t, *counts, next);
            else
                TAILQ_INSERT_TAIL(&hmap->pcy.queue, *counts, next);
        }
    }
    return U_HMAP_ERR_NONE;
    
err:
    return U_HMAP_ERR_FAIL;
}

/**
 * \brief   Insert an object into the hmap
 * 
 * Insert a (key, val) pair \a obj into \a hmap. Such object should be created
 * with u_hmap_o_new(). The user is responsible for allocation of keys and
 * values unless U_HMAP_OPTS_OWNSDATA is set. If a value is overwritten, the \a
 * old value is returned (only if data is owned by user).
 * 
 * \param hmap      hmap object
 * \param obj       key to be inserted 
 * \param old       returned old value
 * 
 * \return U_HMAP_ERR_NONE on success, U_HMAP_ERR_FAIL on failure
 */
int u_hmap_put (u_hmap_t *hmap, u_hmap_o_t *obj, u_hmap_o_t **old)
{
    u_hmap_o_t *o;
    u_hmap_e_t *x;
    int comp;
	size_t hash;

    dbg_err_if (hmap == NULL);
    dbg_err_if (obj == NULL);

    if (old)
        *old = NULL;

    if (hmap->sz >= hmap->threshold) {
        u_dbg("hmap full");
        if (hmap->opts->policy == U_HMAP_PCY_NONE) {
            dbg_err_if (__resize(hmap));
        } else {
            u_dbg("freeing according to policy %d", hmap->opts->policy);
            dbg_err_if (hmap->pcy.pop(hmap, old));
        }
    }

    hash = hmap->opts->f_hash(obj->key, hmap->size);

    if (hmap->opts->f_hash != &__f_hash &&
            !(hmap->opts->options & U_HMAP_OPTS_HASH_STRONG)) {
        enum { MAX_INT = 20 };
        char h[MAX_INT];

        u_snprintf(h, MAX_INT, "%u", hash);
        hash = __f_hash(h, hmap->size);
    }

    if (hmap->opts->policy != U_HMAP_PCY_NONE &&
            hmap->sz >= hmap->opts->max) 
    {
        u_dbg("Cache full - freeing according to policy %d", 
                hmap->opts->policy);
        hmap->pcy.pop(hmap, old);
    }

	x = &hmap->hmap[hash];

    switch (hmap->opts->type) 
    {
        case U_HMAP_TYPE_CHAIN:

            if (LIST_EMPTY(x))
            {
                LIST_INSERT_HEAD(x, obj, next);
                goto end;
            } else {
                LIST_FOREACH(o, x, next) 
                {
                    /* object already hmapd */
                    if ((comp = hmap->opts->f_comp(obj->key, o->key)) == 0) 
                    { 
                        /* overwrite */
                        if (!(hmap->opts->options & U_HMAP_OPTS_NO_OVERWRITE))
                        {
                            LIST_INSERT_AFTER(o, obj, next);
                            LIST_REMOVE(o, next);
                            hmap->sz--;
                            /* XXX pop from policy queue */

                            if (hmap->opts->options & U_HMAP_OPTS_OWNSDATA)
                                __o_free(hmap, o);
                            else
                                if (old)
                                    *old = o;

                            goto end;

                        /* don't overwrite */
                        } else {

                            if (hmap->opts->options & U_HMAP_OPTS_OWNSDATA)
                                __o_free(hmap, obj);                          
                            else
                                if (old) 
                                    *old = obj; 
                            
                            return U_HMAP_ERR_EXISTS; 
                        }
                    } else { 
                        if (comp < 0) 
                        {
                            LIST_INSERT_BEFORE(o, obj, next); 
                            goto end;
                        } else if (!LIST_NEXT(o, next)) {
                            LIST_INSERT_AFTER(o, obj, next);
                            goto end;
                        }
                    }
                }
            }
            break;

        case U_HMAP_TYPE_LINEAR:

            {
                size_t last = ((hash + hmap->size -1) % hmap->size);

                for (; hash != last; hash = ((hash+1) % hmap->size), 
                        x = &hmap->hmap[hash])
                {
                    if (LIST_EMPTY(x)) 
                    {
                        LIST_INSERT_HEAD(x, obj, next);
                        goto end;

                    } else {

                        o = LIST_FIRST(x);

                        /* object already hmapd */
                        if (hmap->opts->f_comp(o->key, obj->key) == 0)
                        {
                            /* overwrite */
                            if (!(hmap->opts->options & U_HMAP_OPTS_NO_OVERWRITE)) 
                            {
                                LIST_INSERT_AFTER(o, obj, next);
                                LIST_REMOVE(o, next);
                                hmap->sz--;
                                /* XXX pop from policy queue */

                                if (hmap->opts->options & U_HMAP_OPTS_OWNSDATA)
                                    __o_free(hmap, obj);
                                else 
                                    if (old)
                                        *old = obj;

                                goto end;

                            /* don't overwrite */
                            } else {

                                if (hmap->opts->options & U_HMAP_OPTS_OWNSDATA)
                                    __o_free(hmap, obj);
                                else 
                                    if (old)
                                        *old = obj;

                                return U_HMAP_ERR_EXISTS;
                            }
                        }
                    }
                }
            }
            break;
    }

err:
    return U_HMAP_ERR_FAIL;

end:
    hmap->sz++;

    if (hmap->pcy.ops & U_HMAP_PCY_OP_PUT)
        hmap->pcy.push(hmap, obj, &obj->pqe);

    return U_HMAP_ERR_NONE;
}

/**
 * \brief   Retrieve an object from the hmap
 * 
 * Retrieve object with given \a key from \a hmap. On success the requested
 * object is returned in \a obj. The object is not removed from the hmap, so
 * ownership of the object is not returned to the user.
 * 
 * \param hmap      hmap object
 * \param key       key to be retrieved 
 * \param obj       returned object
 * 
 * \return U_HMAP_ERR_NONE on success, U_HMAP_ERR_FAIL on failure
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
        hmap->pcy.push(hmap, *obj, &(*obj)->pqe);
        
    return U_HMAP_ERR_NONE;

err:
    return U_HMAP_ERR_FAIL;
}

/**
 * \brief   Perform an operation on all objects
 * 
 * Execute function \a f on all objects within \a hmap. These functions should 
 * return U_HMAP_ERR_NONE on success, and take an object as a parameter.
 * 
 * \param hmap      hmap object
 * \param f         function    
 * 
 * \return U_HMAP_ERR_NONE on success, U_HMAP_ERR_FAIL on failure
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
 * Execute function \a f on all objects within \a hmap supplying \a arg
 * as custom argument.  These functions should return \c U_HMAP_ERR_NONE on 
 * success, and take an hmap object and \a arg as parameters.
 * 
 * \param hmap      hmap object
 * \param f         function 
 * \param arg       custom arg that will be supplied to \a f
 * 
 * \return U_HMAP_ERR_NONE on success, U_HMAP_ERR_FAIL on failure
 */
int u_hmap_foreach_arg (u_hmap_t *hmap, int f(const void *val, const void *arg), 
        void *arg)
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
 * Execute function \a f on all objects within \a hmap. These functions should 
 * return U_HMAP_ERR_NONE on success, and take an object as a parameter.
 * 
 * \param hmap      hmap object
 * \param f         function, must accept key and val params   
 * 
 * \return U_HMAP_ERR_NONE on success, U_HMAP_ERR_FAIL on failure
 */
int u_hmap_foreach_keyval(u_hmap_t *hmap, int f(const void *key, const void *val))
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
 * \brief   Deallocate hmap
 * 
 * Deallocate \a hmap along with all hmapd objects (unless U_HMAP_OPTS_OWNSDATA
 * is set). Objects are freed via free() by default or using the custom
 * deallocation function passed in the hmap options. 
 * 
 * \param hmap      hmap object
 */
void u_hmap_free (u_hmap_t *hmap)
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

    u_free(hmap->hmap);

    /* free the policy queue */
    while ((data = TAILQ_FIRST(&hmap->pcy.queue)) != NULL) 
    {
        TAILQ_REMOVE(&hmap->pcy.queue, data, next);
        __q_o_free(data);
    }

    u_free(hmap->opts);
    u_free(hmap);

    return;
}

/**
 * \brief   Create new hmap options
 * 
 * Create a new hmap options object and save its pointer to \a *opts. 
 * The fields within the object can be manipulated publicly according to the
 * description in the header file.
 * 
 * \param opts      on success contains the hmap options object
 * 
 * \return U_HMAP_ERR_NONE on success, U_HMAP_ERR_FAIL on failure
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
 * \brief Copy options
 *
 * Perform a deep copy of options object \a from to \a to.
 *
 * \param to        destination options
 * \param from      source options
 * 
 * \return U_HMAP_ERR_NONE on success, U_HMAP_ERR_FAIL on failure
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
 * \brief   Initialise hmap options
 * 
 * Set allocated hmap options to default values
 * 
 * \param opts      hmap options object
 */
void u_hmap_opts_init (u_hmap_opts_t *opts)
{
    dbg_ifb (opts == NULL) return;

    opts->size = U_HMAP_MAX_SIZE;
    opts->type = U_HMAP_TYPE_CHAIN;
    opts->max = U_HMAP_MAX_ELEMS;
    opts->policy = U_HMAP_PCY_NONE;
    opts->options = 0;
    opts->f_hash = &__f_hash;
    opts->f_comp = &__f_comp;
    opts->f_free = NULL;
    opts->key_type = U_HMAP_OPTS_DATATYPE_STRING;
    opts->val_type = U_HMAP_OPTS_DATATYPE_POINTER;
    opts->f_key_free = NULL;
    opts->f_val_free = NULL;
    opts->f_str = NULL;
    opts->key_sz = sizeof(void *);
    opts->val_sz = sizeof(void *);
    opts->easy = 0;
    
    return;
}

/**
 * \brief   Deallocate hmap options
 * 
 * Deallocate hmap options object \a opts.
 * 
 * \param opts      hmap options 
 */
void u_hmap_opts_free (u_hmap_opts_t *opts)
{
    dbg_ifb (opts == NULL) return;

    u_free(opts);
}

/**
 * \brief Debug options
 * 
 * Print out information on option settings.
 * 
 * \param opts  options object
 */
void u_hmap_opts_dbg (u_hmap_opts_t *opts)
{
    dbg_ifb (opts == NULL) return;

    u_dbg("[hmap options]");
    u_dbg("easy: %d", opts->easy);
    u_dbg("size: %u", opts->size);
    u_dbg("max: %u", opts->max);
    u_dbg("policy: %s", __pcy2str(opts->policy));
    u_dbg("ownsdata: %d, f_free: %p, f_key_free: %p, f_val_free: %p", 
            opts->options & U_HMAP_OPTS_OWNSDATA,
            opts->f_free, opts->f_key_free, opts->f_val_free);
    u_dbg("f_str: %p", opts->f_str);
    u_dbg("no_overwrite: %d", opts->options & U_HMAP_OPTS_NO_OVERWRITE);
    u_dbg("key type: %d, val type: %d", opts->key_type, opts->val_type);
}

/**
 * \brief   Create a data object
 *
 * Creates a new (key, value) tuple to be inserted into a hmap. By default, the
 * user is responsible for allocation and deallocation of these objects and
 * their content. If the option U_HMAP_OPTS_OWNSDATA is set 
 *
 * \param hmap      reference to parent hmap
 * \param key       pointer to the key
 * \param val       pointer to the oject
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
    
    if (hmap->opts->options & U_HMAP_OPTS_OWNSDATA) {

        /* internalise key */
        switch (hmap->opts->key_type) {
            case U_HMAP_OPTS_DATATYPE_POINTER:
                obj->key = (void *) key;
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
        switch (hmap->opts->val_type) {
            case U_HMAP_OPTS_DATATYPE_POINTER:
                obj->val = (void *) val;
                break;
            case U_HMAP_OPTS_DATATYPE_STRING:
                dbg_err_if ((obj->val = u_strdup((const char *) val)) == NULL);
                break;
            case U_HMAP_OPTS_DATATYPE_OPAQUE:
                dbg_err_if ((obj->val = u_zalloc(hmap->opts->val_sz)) == NULL);
                memcpy(obj->val, val, hmap->opts->val_sz);
                break;
        }

    } else {  /* data owned by user - do not internalise, just set pointers */
        
        obj->key = (void *) key;
        obj->val = (void *) val;
    }

    obj->pqe = NULL;

    return obj;
 
err:
    U_FREEF(obj, u_hmap_o_free);

    return NULL;
}

/** 
 * \brief  Free a data object 
 *
 * Frees a data object (without freeing its content). This function should only
 * be used if U_HMAP_OPTS_OWNSDATA is not set to free objects allocated with
 * u_hmap_o_new(). If U_HMAP_OPTS_OWNSDATA is set, the data is freed
 * automatically by the hashmap by using the default free function or the
 * overridden f_free().
 *
 * \param obj       hmap object
 */
void u_hmap_o_free (u_hmap_o_t *obj)
{
    dbg_ifb (obj == NULL) return;

    u_free(obj);
}

/**
 *      \}
 */

/* Free a data object including content (only if U_HMAP_OPTS_OWNSDATA) */
static void __o_free (u_hmap_t *hmap, u_hmap_o_t *obj)
{
    dbg_ifb (hmap == NULL) return;
    dbg_ifb (obj == NULL) return;

    if (hmap->opts->f_free) {
        hmap->opts->f_free(obj);
    } else {
        switch (hmap->opts->key_type) {
            case U_HMAP_OPTS_DATATYPE_POINTER:
                if (hmap->opts->f_key_free)
                    hmap->opts->f_key_free(obj->key);
                break;
            case U_HMAP_OPTS_DATATYPE_STRING:
            case U_HMAP_OPTS_DATATYPE_OPAQUE:
                u_free(obj->key);
                break;
        }
        switch (hmap->opts->val_type) {
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

/* Allocate a new queue data object */
static u_hmap_q_t *__q_o_new (const void *key)
{
    u_hmap_q_t *data = NULL;

    dbg_return_if (key == NULL, NULL);

    dbg_err_sif ((data = (u_hmap_q_t *)
                u_zalloc(sizeof(u_hmap_q_t))) == NULL);

    data->key = (void *) key;
    data->o = NULL;
    
    return data;

err:
    u_free(data);
    return NULL;
}

/* Free a data queue object */
static void __q_o_free (u_hmap_q_t *data)
{
    dbg_ifb (data == NULL) return;

    u_free(data->o);
    u_free(data);
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
            return "fifo";
        case U_HMAP_PCY_LFU:
            return "lfu";
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
    
    for (i = *idx; i < sizeof(primes)/sizeof(size_t); ++i) {
        if (primes[i] >= sz) {
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

int u_hmap_opts_set_size (u_hmap_opts_t *opts, int sz)
{
    dbg_err_if (opts == NULL || sz <= 0);

    opts->size = sz;

    return 0;
err:
    return ~0;
}

int u_hmap_opts_set_max (u_hmap_opts_t *opts, int max)
{
    dbg_err_if (opts == NULL || max <= 0);

    opts->max = max;

    return 0;
err:
    return ~0;
}

int u_hmap_opts_set_type (u_hmap_opts_t *opts, int type)
{
    dbg_err_if (opts == NULL);

    dbg_err_if (type < 0 || type > U_HMAP_TYPE_LAST);

    opts->type = type;

    return 0;
err:
    return ~0;
}

int u_hmap_opts_set_policy (u_hmap_opts_t *opts, int policy)
{
    dbg_err_if (opts == NULL || policy < 0 || policy > U_HMAP_TYPE_LAST);

    opts->policy = policy; 

    return 0;
err:
    return ~0;
}

/* not valid for simplified interface */
int u_hmap_opts_set_options (u_hmap_opts_t *opts, int options)
{
    dbg_err_if (opts == NULL || opts->easy); 

    opts->options |= options;

    return 0;
err:
    return ~0;
}

/* not valid for simplified interface */
int u_hmap_opts_set_hashfunc (u_hmap_opts_t *opts, 
        size_t (*f_hash)(const void *key, size_t buckets))
{
    dbg_err_if (opts == NULL || f_hash == NULL || opts->easy); 

    opts->f_hash = f_hash;

    return 0;
err:
    return ~0;
}

/* not valid for simplified interface */
int u_hmap_opts_set_compfunc (u_hmap_opts_t *opts, 
        int (*f_comp)(const void *k1, const void *k2))
{
    dbg_err_if (opts == NULL || f_comp == NULL || opts->easy); 

    opts->f_comp = f_comp;

    return 0;
err:
    return ~0;
}

/* not valid for simplified interface */
int u_hmap_opts_set_freefunc (u_hmap_opts_t *opts, 
        void (*f_free)(u_hmap_o_t *obj))
{
    dbg_err_if (opts == NULL || opts->easy); 

    opts->f_free = f_free;

    return 0;
err:
    return ~0;
}

int u_hmap_opts_set_strfunc (u_hmap_opts_t *opts, 
        u_string_t *(*f_str)(u_hmap_o_t *obj))
{
    dbg_err_if (opts == NULL);

    opts->f_str = f_str;

    return 0;
err:
    return ~0;
}

int u_hmap_opts_set_key_type (u_hmap_opts_t *opts, 
        u_hmap_options_datatype_t type)
{
    dbg_err_if (opts == NULL ||
            (type < 0 || type > U_HMAP_OPTS_DATATYPE_LAST));

    /* not valid for simplified interface */
    dbg_err_if (opts->easy);

    opts->key_type = type;

    return 0;
err:
    return ~0;
}

int u_hmap_opts_set_key_sz (u_hmap_opts_t *opts, size_t sz)
{
    dbg_err_if (opts == NULL || 
            sz == 0);

    /* not valid for simplified interface */
    dbg_err_if (opts->easy);

    opts->key_sz = sz;

    return 0;
err:
    return ~0;
}

int u_hmap_opts_set_key_freefunc (u_hmap_opts_t *opts, 
        void (*f_free)(const void *key))
{
    dbg_err_if (opts == NULL);

    /* not valid for simplified interface */
    dbg_err_if (opts->easy);

    opts->f_key_free = f_free;

    return 0;
err:
    return ~0;
}

int u_hmap_opts_set_val_type (u_hmap_opts_t *opts, 
        u_hmap_options_datatype_t type)
{
    dbg_err_if (opts == NULL ||
            (type < 0 || type > U_HMAP_OPTS_DATATYPE_LAST));

    opts->val_type = type;

    return 0;
err:
    return ~0;
}

int u_hmap_opts_set_val_sz (u_hmap_opts_t *opts, size_t sz)
{
    dbg_err_if (opts == NULL || 
            sz == 0);

    opts->val_sz = sz;

    return 0;
err:
    return ~0;
}

int u_hmap_opts_set_val_freefunc (u_hmap_opts_t *opts, void (*f_free)(void *val))
{
    dbg_err_if (opts == NULL);

    opts->f_val_free = f_free;

    return 0;
err:
    return ~0;
}

