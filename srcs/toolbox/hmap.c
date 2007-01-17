/* $Id: hmap.c,v 1.8 2007/01/17 10:32:21 stewy Exp $ */

#include <sys/types.h>
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


/* policy queue object */
struct u_hmap_q_s 
{
    void *key,
         *o;
    TAILQ_ENTRY(u_hmap_q_s) next;
};

/* hmap policy representation */
struct u_hmap_pcy_s 
{
    int (*pop)(struct u_hmap_s *hmap, u_hmap_o_t **obj); 
    int (*push)(struct u_hmap_s *hmap, u_hmap_o_t *obj,
            u_hmap_q_t **data); 
    TAILQ_HEAD(u_hmap_q_h_s, u_hmap_q_s) queue;
    enum {
        U_HMAP_PCY_OP_PUT = 0x1,
        U_HMAP_PCY_OP_GET = 0x2
    } ops;
};

/* hmap representation */
struct u_hmap_s 
{
    u_hmap_opts_t *opts;        /* hmap options */

    size_t sz,                  /* current size */
           size;                /* array size */

    LIST_HEAD(u_hmap_e_s, u_hmap_o_s) *hmap;    /* the hashmap */

    struct u_hmap_pcy_s pcy;    /* discard policy */
};


static int _get (u_hmap_t *hmap, void *key, 
        u_hmap_o_t **o);

static int _opts_check (u_hmap_opts_t *opts);
static int _pcy_setup (u_hmap_t *hmap);
static const char *_pcy2str(u_hmap_pcy_t policy);

static void _o_free (u_hmap_t *hmap, u_hmap_o_t *obj);

static u_hmap_q_t *_q_o_new (void *key);
static void _q_o_free (u_hmap_q_t *s);

static size_t _f_hash (void *key, size_t size);
static int _f_comp (void *k1, void *k2);
static void _f_free (u_hmap_o_t *obj);
static u_string_t *_f_str (u_hmap_o_t *obj);

static int _queue_push (u_hmap_t *hmap, u_hmap_o_t *obj, 
        u_hmap_q_t **data);
static int _queue_push_count (u_hmap_t *hmap, u_hmap_o_t *obj,
        u_hmap_q_t **counts);
static int _queue_pop_front (u_hmap_t *hmap, u_hmap_o_t **obj);
static int _queue_pop_back (u_hmap_t *hmap, u_hmap_o_t **obj);


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
static size_t _f_hash (void *key, size_t size)
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
static int _f_comp (void *k1, void *k2) 
{
    dbg_ifb (k1 == NULL) return -1;    
    dbg_ifb (k2 == NULL) return -1;  
    
    return strcmp((char *)k1, (char *)k2);
}

/* Default function for freeing hmap objects  */
static void _f_free (u_hmap_o_t *obj)
{
    dbg_ifb (obj == NULL) return;

    u_free(obj->key); 
    u_free(obj->val); 
}

/* Default string representation of objects */
static u_string_t *_f_str (u_hmap_o_t *obj)
{
    enum { MAX_OBJ_STR = 256 };
    char buf[MAX_OBJ_STR];
    u_string_t *s = NULL;

    dbg_err_if (obj == NULL);

    char *key = (char *) obj->key,
         *val = (char *) obj->val;

    dbg_err_if (u_snprintf(buf, MAX_OBJ_STR, "[%s:%s]", key, val));    
    dbg_err_if (u_string_create(buf, strlen(buf)+1, &s));

    return s;

err:
    return NULL;
}

/* Check validity of options */
static int _opts_check (u_hmap_opts_t *opts)
{
    dbg_err_if (opts == NULL);

    dbg_err_if (opts->max_size == 0);
    dbg_err_if (opts->max_elems == 0);
    dbg_err_if (opts->policy < U_HMAP_PCY_NONE || 
            opts->policy > U_HMAP_PCY_LFU);
    dbg_err_if (opts->f_hash == NULL);
    dbg_err_if (opts->f_comp == NULL);

    return U_HMAP_ERR_NONE;

err:
    return U_HMAP_ERR_FAIL;
}

/* Setup policy parameters */
static int _pcy_setup (u_hmap_t *hmap) 
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
            hmap->pcy.push = _queue_push;
            hmap->pcy.pop = _queue_pop_back;
            hmap->pcy.ops = U_HMAP_PCY_OP_PUT | U_HMAP_PCY_OP_GET;
            break;
        case U_HMAP_PCY_FIFO:
            hmap->pcy.push = _queue_push;
            hmap->pcy.pop = _queue_pop_back;
            hmap->pcy.ops = U_HMAP_PCY_OP_PUT;
            break;
        case U_HMAP_PCY_LFU:
            hmap->pcy.push = _queue_push_count;
            hmap->pcy.pop = _queue_pop_front;
            hmap->pcy.ops = U_HMAP_PCY_OP_PUT | U_HMAP_PCY_OP_GET;
            break;
        default:
            dbg("Invalid policy: %d", hmap->opts->policy);
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
 * \param hmap      on success contains the hmap options object
 * 
 * \return U_HMAP_ERR_NONE on success, U_HMAP_ERR_FAIL on failure
 */
int u_hmap_new (u_hmap_opts_t *opts, u_hmap_t **hmap)
{
    size_t i;
    u_hmap_t *c = NULL;

    /* allow (opts == NULL) */
    dbg_return_if (hmap == NULL, ~0);
   
    dbg_return_sif ((c = (u_hmap_t *) u_zalloc(sizeof(u_hmap_t))) == NULL, ~0);
    
    dbg_err_if (u_hmap_opts_new(&c->opts));
    if (opts)
    {
        dbg_err_if (u_hmap_opts_copy(c->opts, opts));
        dbg_err_if (_opts_check(c->opts));
    }
    u_hmap_opts_dbg(c->opts);
    dbg_err_if (_pcy_setup(c));

    c->sz = 0;
    c->size = c->opts->max_size;
    dbg_err_sif ((c->hmap = (struct u_hmap_e_s *) 
                u_zalloc(sizeof(struct u_hmap_e_s) * 
                    c->size)) == NULL);

    /* initialise entries */
    for (i = 0; i < c->size; i++)
        LIST_INIT(&c->hmap[i]);

    TAILQ_INIT(&c->pcy.queue);

    *hmap = c;

    return U_HMAP_ERR_NONE;

err:
    u_free(c);
    *hmap = NULL;    
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

    dbg ("<hmap>");
    for (i = 0; i < hmap->size; i++) 
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
        dbg(u_string_c(s));
        dbg_ifb (u_string_free(s)) return;
    }
    dbg("</hmap>");
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
int u_hmap_del (u_hmap_t *hmap, void *key, u_hmap_o_t **obj) 
{
    u_hmap_o_t *o = NULL;

    dbg_err_if (hmap == NULL);
    dbg_err_if (key == NULL);

    if (obj)
        *obj = NULL;

    if (_get(hmap, key, &o))
        return U_HMAP_ERR_FAIL;

    dbg_err_if (o == NULL);
    LIST_REMOVE(o, next);

    if (hmap->opts->options & U_HMAP_OPTS_OWNSDATA)
        _o_free(hmap, o);
    else
        if (obj)
            *obj = o;

    hmap->sz--;

    return U_HMAP_ERR_NONE;
    
err:
    return U_HMAP_ERR_FAIL;
}

/* Retrieve an hmap element given a key */
static int _get (u_hmap_t *hmap, void *key, 
                       u_hmap_o_t **o)
{
    u_hmap_o_t *obj;
    struct u_hmap_e_s *x;
    int comp;
	size_t hash;

    dbg_err_if (hmap == NULL);
    dbg_err_if (key == NULL);
    dbg_err_if (o == NULL);

	hash = hmap->opts->f_hash(key, hmap->size);

	if (hmap->opts->f_hash != &_f_hash && 
			!(hmap->opts->options & U_HMAP_OPTS_HASH_STRONG)) {
		enum { MAX_INT = 20 };
		char h[MAX_INT];

		u_snprintf(h, MAX_INT, "%u", hash);
		hash = _f_hash(h, hmap->size);
	}

	x = &hmap->hmap[hash];

    LIST_FOREACH(obj, x, next) 
    {
        if ((comp = hmap->opts->f_comp(key, obj->key)) == 0) /* object found */
        { 
            *o = obj;
            return U_HMAP_ERR_NONE;
        } else if (comp < 0) { /* cannot be in list (ordered) */
            *o = NULL;
            break;
        }
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
    dbg(u_string_c(s));
    dbg_if (u_string_free(s));

    return;
    
 err:
    U_FREEF(s, u_string_free);
    return;
}

/* pop the front of an object queue */
static int _queue_pop_front (u_hmap_t *hmap, u_hmap_o_t **obj)
{
    u_hmap_q_t *first;

    dbg_err_if (hmap == NULL);

    dbg_err_if ((first = TAILQ_FIRST(&hmap->pcy.queue)) == NULL);
    dbg_err_if (u_hmap_del(hmap, first->key, obj));
    TAILQ_REMOVE(&hmap->pcy.queue, first, next);
    _q_o_free(first);

    return U_HMAP_ERR_NONE;

err:
    return U_HMAP_ERR_FAIL;
}

/* pop the back of an object queue */
static int _queue_pop_back (u_hmap_t *hmap, u_hmap_o_t **obj)
{
    u_hmap_q_t *last;

    dbg_err_if (hmap == NULL);

    dbg_err_if ((last = TAILQ_LAST(&hmap->pcy.queue, u_hmap_q_h_s))
            == NULL);
    dbg_err_if (u_hmap_del(hmap, last->key, obj));
    TAILQ_REMOVE(&hmap->pcy.queue, last, next);
    _q_o_free(last);
    
    return U_HMAP_ERR_NONE;

err: 
    return U_HMAP_ERR_FAIL;
}

/* push object data onto queue */
static int _queue_push (u_hmap_t *hmap, u_hmap_o_t *obj,
        u_hmap_q_t **data)
{
    u_hmap_q_t *new;

    dbg_err_if (hmap == NULL);
    dbg_err_if (obj == NULL);
    dbg_err_if (data == NULL);

    if (*data == NULL) 
    {  /* no reference to queue entry */
        dbg_err_if ((new = _q_o_new(obj->key)) == NULL);
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
static int _queue_push_count (u_hmap_t *hmap, u_hmap_o_t *obj, 
        u_hmap_q_t **counts)
{
    u_hmap_q_t *new, *t;
    int *count;

    dbg_err_if (hmap == NULL);
    dbg_err_if (obj == NULL);
    dbg_err_if (counts == NULL);

    if (*counts == NULL) /* no reference to queue entry */
    {  
        dbg_err_if ((new = _q_o_new(obj->key)) == NULL);
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
    struct u_hmap_e_s *x;
    int comp;
	size_t hash;

    dbg_err_if (hmap == NULL);
    dbg_err_if (obj == NULL);

    if (old)
        *old = NULL;

    hash = hmap->opts->f_hash(obj->key, hmap->size);

    if (hmap->opts->f_hash != &_f_hash &&
            !(hmap->opts->options & U_HMAP_OPTS_HASH_STRONG)) {
        enum { MAX_INT = 20 };
        char h[MAX_INT];

        u_snprintf(h, MAX_INT, "%u", hash);
        hash = _f_hash(h, hmap->size);
    }

    if (hmap->opts->policy != U_HMAP_PCY_NONE &&
            hmap->sz >= hmap->opts->max_elems) 
    {
        dbg("Cache full - freeing according to policy %d", hmap->opts->policy);
        hmap->pcy.pop(hmap, old);
    }

	x = &hmap->hmap[hash];

    if (LIST_EMPTY(x) || (hmap->opts->options & U_HMAP_OPTS_NO_ORDERING)) 
    {
        LIST_INSERT_HEAD(x, obj, next);
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

                    if (hmap->opts->options & U_HMAP_OPTS_OWNSDATA)
                        _o_free(hmap, o);
                    else
                        if (old)
                            *old = o;

                    goto end;

                /* don't overwrite */
                } else {

                    if (hmap->opts->options & U_HMAP_OPTS_OWNSDATA)
                        _o_free(hmap, obj);                          
                    else
                        if (old) 
                            *old = obj; 
                    
                    return U_HMAP_ERR_EXISTS; 
                }
            } else { 
                if (comp < 0) 
                {
                    LIST_INSERT_BEFORE(o, obj, next); 
                    break;
                } else if (!LIST_NEXT(o, next)) {
                    LIST_INSERT_AFTER(o, obj, next);
                    break;
                }
            }
        }
    }
   
    hmap->sz++;

end:
    if (hmap->pcy.ops & U_HMAP_PCY_OP_PUT)
        hmap->pcy.push(hmap, obj, &obj->pqe);

    return U_HMAP_ERR_NONE;

err:
    return U_HMAP_ERR_FAIL;
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
int u_hmap_get (u_hmap_t *hmap, void *key, u_hmap_o_t **obj) 
{
    dbg_err_if (hmap == NULL);
    dbg_err_if (key == NULL);
    dbg_err_if (obj == NULL);

    if (_get(hmap, key, obj)) 
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
int u_hmap_foreach (u_hmap_t *hmap, int f(void *val))
{
    u_hmap_o_t *obj;
    size_t i;

    dbg_err_if (hmap == NULL);
    dbg_err_if (f == NULL);

    for (i = 0; i < hmap->size; i++) 
    {
        LIST_FOREACH(obj, &hmap->hmap[i], next)
            dbg_err_if (f(obj->val));
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
    for (i = 0; i < hmap->size; i++) 
    {
        while ((obj = LIST_FIRST(&hmap->hmap[i])) != NULL) 
        {
            LIST_REMOVE(obj, next);
            _o_free(hmap, obj);
        }
    }

    u_free(hmap->hmap);

    /* free the policy queue */
    while ((data = TAILQ_FIRST(&hmap->pcy.queue)) != NULL) 
    {
        TAILQ_REMOVE(&hmap->pcy.queue, data, next);
        _q_o_free(data);
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
    
    /* set defaults */
    o->max_size = U_HMAP_MAX_SIZE;
    o->max_elems = U_HMAP_MAX_ELEMS;
    o->policy = U_HMAP_PCY_NONE;
    o->options = 0;
    o->f_hash = &_f_hash;
    o->f_comp = &_f_comp;
    o->f_free = &_f_free;
    o->f_str = &_f_str;
    
    *opts = o;
    
    return U_HMAP_ERR_NONE;
err:
    *opts = NULL;
    return U_HMAP_ERR_FAIL;
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
 * \brief   Copy hmap options
 * 
 * Copy hmap options \a from to \a to.
 * 
 * \param opts      hmap options 
 */
int u_hmap_opts_copy (u_hmap_opts_t *to, u_hmap_opts_t *from)
{
    dbg_err_if (to == NULL);
    dbg_err_if (from == NULL);

    memcpy(to, from, sizeof(u_hmap_opts_t));

    return 0;

err:
    return ~0;
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

    dbg("[hmap options]");
    dbg("max size: %u", opts->max_size);
    dbg("max elems: %u", opts->max_elems);
    dbg("policy: %s", _pcy2str(opts->policy));
    dbg("ownsdata: %d, &f_free: %x", 
            (opts->options & U_HMAP_OPTS_OWNSDATA)>0,
            &opts->f_free);
    dbg("no_overwrite: %d", (opts->options & U_HMAP_OPTS_NO_OVERWRITE)>0);
    dbg("no_ordering: %d", (opts->options & U_HMAP_OPTS_NO_ORDERING)>0);
}

/**
 * \brief   Create a data object
 *
 * Creates a new (key, value) tuple to be inserted into a hmap. By default, the
 * user is responsible for allocation and deallocation of these objects and
 * their content. If the option U_HMAP_OPTS_OWNSDATA is set 
 *
 * \param key       pointer to the key
 * \param val       pointer to the oject
 *
 * \return pointer to a new u_hmap_o_t 
 */
u_hmap_o_t *u_hmap_o_new (void *key, void *val)
{
    u_hmap_o_t *obj = NULL;

    dbg_return_if (key == NULL, NULL);
    dbg_return_if (val == NULL, NULL);

    dbg_err_sif ((obj = (u_hmap_o_t *) 
                u_zalloc(sizeof(u_hmap_o_t))) == NULL);
    
    obj->key = key;
    obj->val = val;
    obj->pqe = NULL;

    return obj;
 
err:
    u_free(obj);
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

/* Free a data object including content if U_HMAP_OPTS_OWNSDATA */
static void _o_free (u_hmap_t *hmap, u_hmap_o_t *obj)
{
    dbg_ifb (hmap == NULL) return;
    dbg_ifb (obj == NULL) return;

    if (hmap->opts->options & U_HMAP_OPTS_OWNSDATA) 
    {
        if (hmap->opts->f_free)
            hmap->opts->f_free(obj);

        u_hmap_o_free(obj); 
    }
}

/* Allocate a new queue data object */
static u_hmap_q_t *_q_o_new (void *key)
{
    u_hmap_q_t *data = NULL;

    dbg_return_if (key == NULL, NULL);

    dbg_err_sif ((data = (u_hmap_q_t *)
                u_zalloc(sizeof(u_hmap_q_t))) == NULL);

    data->key = key;
    data->o = NULL;
    
    return data;

err:
    u_free(data);
    return NULL;
}

/* Free a data queue object */
static void _q_o_free (u_hmap_q_t *data)
{
    dbg_ifb (data == NULL) return;

    u_free(data->o);
    u_free(data);
}

/* Get a string representation of a policy */
static const char *_pcy2str (u_hmap_pcy_t policy)
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

/**
 *      \}
 */
