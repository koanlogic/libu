/* $Id: hmap.c,v 1.7 2006/04/20 09:53:53 tat Exp $ */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <u/memory.h>
#include <u/carpal.h>
#include <u/queue.h>
#include <u/str.h>
#include <u/hmap.h>

/**
 *  \defgroup hmap Map 
 *  \{
 */

/* default limits handled by policies */
#define U_HMAP_MAX_SIZE      512
#define U_HMAP_MAX_ELEMS     U_HMAP_MAX_SIZE


/* policy queue object */
struct u_hmap_queue_o_s 
{
    char *key;
    void *o;
    TAILQ_ENTRY(u_hmap_queue_o_s) next;
};

/* hmap object abstraction */
struct u_hmap_o_s 
{
    char *key;
    void *val;
    LIST_ENTRY(u_hmap_o_s) next;
    struct u_hmap_queue_o_s *pqe; 
};

/* hmap policy representation */
struct u_hmap_pcy_s 
{
    int (*pop)(struct u_hmap_s *hmap); 
    int (*push)(struct u_hmap_s *hmap, struct u_hmap_o_s *obj,
            struct u_hmap_queue_o_s **data); 
    TAILQ_HEAD(u_hmap_queue_h_s, u_hmap_queue_o_s) queue;
    enum {
        U_HMAP_PCY_OP_PUT = 0x1,
        U_HMAP_PCY_OP_GET = 0x2
    } ops;
};

/* hmap representation */
struct u_hmap_s 
{
    u_hmap_opts_t *opts;
    size_t size;
    LIST_HEAD(u_hmap_e_s, u_hmap_o_s) *hmap;
    struct u_hmap_pcy_s pcy;
};


static int _get (u_hmap_t *hmap, const char *key, 
        struct u_hmap_o_s **o);

static int _opts_check (u_hmap_opts_t *opts);
static int _pcy_setup (u_hmap_t *hmap);

static struct u_hmap_o_s *_o_new (const char *key, void *val);
static void _o_free (u_hmap_t *hmap, struct u_hmap_o_s *obj);

static struct u_hmap_queue_o_s *_data_o_new(const char *key);
static void _data_o_free (struct u_hmap_queue_o_s *s);

static size_t _f_hash (const char *key, size_t size);
static int _f_comp (const char *k1, const char *k2);
static void _f_free (void *val);

static int _queue_push (u_hmap_t *hmap, struct u_hmap_o_s *obj, 
        struct u_hmap_queue_o_s **data);
static int _queue_push_count (u_hmap_t *hmap, struct u_hmap_o_s *obj,
        struct u_hmap_queue_o_s **counts);
static int _queue_pop_front (u_hmap_t *hmap);
static int _queue_pop_back (u_hmap_t *hmap);


static size_t _f_hash (const char *key, size_t size)
{
    size_t h = 0;
    
    while (*key)
    {
        h += *key++;
        h += (h << 10);
        h ^= (h >> 6);
    }

    h += (h << 3);
    h ^= (h >> 11);

    return (h + (h << 15)) % size;
}

static int _f_comp (const char *k1, const char *k2) 
{
    return strcmp(k1, k2);
}

static void _f_free (void *val)
{
    dbg_ifb (val == NULL) return;

    u_free(val); 
}

static int _opts_check (u_hmap_opts_t *opts)
{
    dbg_err_if (opts == NULL);

    dbg_err_if (opts->max_size == 0);
    dbg_err_if (opts->max_elems == 0);
    dbg_err_if (opts->policy < U_HMAP_PCY_NONE || 
            opts->policy > U_HMAP_PCY_LFU);
    dbg_err_if (opts->f_hash == NULL);
    dbg_err_if (opts->f_comp == NULL);
    dbg_err_if (opts->f_free == NULL);
     
    return 0;

err:
    return ~0;
}

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
            return ~0;
    }
    return 0;
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
 * \return 0 on success, non-zero on failure
 */
int u_hmap_new (u_hmap_opts_t *opts, u_hmap_t **hmap)
{
    size_t i;
    u_hmap_t *c = NULL;

    dbg_return_if (hmap == NULL, ~0);
   
    dbg_return_if ((c = (u_hmap_t *) u_zalloc(sizeof(u_hmap_t))) == NULL, ~0);
    
    if (opts == NULL)
    {
        dbg_err_if (u_hmap_opts_new(&c->opts));
    } else { 
        c->opts = opts;
        dbg_err_if (_opts_check(c->opts));
    }
    dbg_err_if (_pcy_setup(c));

    c->size = 0;
    dbg_err_if ((c->hmap = (struct u_hmap_e_s *) 
                u_zalloc(sizeof(struct u_hmap_e_s) * 
                    c->opts->max_size)) == NULL);

    /* initialise entries */
    for (i = 0; i < c->opts->max_size; i++)
        LIST_INIT(&c->hmap[i]);

    TAILQ_INIT(&c->pcy.queue);

    *hmap = c;

    return 0;

err:
    u_free(c);
    *hmap = NULL;    
    return ~0;
}

void u_hmap_dbg (u_hmap_t *hmap)
{
    enum { MAX_LINE = 255 };
    u_string_t *s = NULL, *st = NULL;
    struct u_hmap_o_s *obj;
    size_t i;

    dbg_ifb (hmap == NULL) return;

    dbg ("<hmap>");
    for (i = 0; i < hmap->opts->max_size; i++) 
    {
        dbg_ifb (u_string_create("", 1, &s)) return;
        dbg_err_if (u_string_clear(s));
        dbg_err_if (u_string_append(s, "|", 1));

        LIST_FOREACH(obj, &hmap->hmap[i], next) 
        {
            dbg_err_if (u_string_append(s, "[", 1));
            dbg_err_if (u_string_append(s, obj->key, strlen(obj->key)));
            dbg_err_if (u_string_append(s, ":", 1));

            if (hmap->opts->f_str == NULL) 
            {
                dbg_err_if (u_string_append(s, "*", 1));
            } else {
                st = hmap->opts->f_str(obj->val);
                dbg_err_if (u_string_append(s, u_string_c(st),
                            u_string_len(st)-1));
            }
            dbg_err_if (u_string_append(s, "]", 1));
        } 
        dbg_err_if (u_string_append(s, "|", 1));
        dbg(u_string_c(s));
        dbg_ifb (u_string_free(s)) return;
    }
    dbg("</hmap>");
    return;

err:
    u_string_free(s);
    return;   
}

/**
 * \brief   Delete an object from the hmap
 * 
 * Delete object with given \a key from \a hmap.
 * 
 * \param hmap      hmap object
 * \param key       key of object to be deleted 
 * 
 * \return 0 on success, non-zero on failure
 */
int u_hmap_del (u_hmap_t *hmap, const char *key) 
{
    struct u_hmap_o_s *obj = NULL;

    dbg_err_if (hmap == NULL);
    dbg_err_if (key == NULL);

    if (_get(hmap, key, &obj))
        return ~0;

    dbg_err_if (obj == NULL);
    LIST_REMOVE(obj, next);
    _o_free(hmap, obj);
    return 0;
    
err:
    return ~0;
}

static int _get (u_hmap_t *hmap, const char *key, 
                       struct u_hmap_o_s **o)
{
    struct u_hmap_o_s *obj;
    struct u_hmap_e_s *x;
    int comp;

    dbg_err_if (hmap == NULL);
    dbg_err_if (key == NULL);
    dbg_err_if (o == NULL);

    x = &hmap->hmap[hmap->opts->f_hash(key, hmap->opts->max_size)];

    LIST_FOREACH(obj, x, next) 
    {
        if ((comp = hmap->opts->f_comp(key, obj->key)) == 0) /* object found */
        { 
            *o = obj;
            return 0;
        } else if (comp < 0) { /* cannot be in list (ordered) */
            *o = NULL;
            break;
        }
    }

err: 
    return ~0;
}

void u_hmap_pcy_dbg (u_hmap_t *hmap)
{
    struct u_hmap_queue_o_s *data;
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
    dbg_if (u_string_free(s));
    return;
}

static int _queue_pop_front (u_hmap_t *hmap)
{
    struct u_hmap_queue_o_s *first;

    dbg_err_if (hmap == NULL);

    dbg_err_if ((first = TAILQ_FIRST(&hmap->pcy.queue)) == NULL);
    dbg_err_if (u_hmap_del(hmap, first->key));
    TAILQ_REMOVE(&hmap->pcy.queue, first, next);
    _data_o_free(first);

    return 0;

err:
    return ~0;
}

static int _queue_pop_back (u_hmap_t *hmap)
{
    struct u_hmap_queue_o_s *last;

    dbg_err_if (hmap == NULL);

    dbg_err_if ((last = TAILQ_LAST(&hmap->pcy.queue, u_hmap_queue_h_s))
            == NULL);
    dbg_err_if (u_hmap_del(hmap, last->key));
    TAILQ_REMOVE(&hmap->pcy.queue, last, next);
    _data_o_free(last);
    
    return 0;

err: 
    return ~0;
}

static int _queue_push (u_hmap_t *hmap, struct u_hmap_o_s *obj,
        struct u_hmap_queue_o_s **data)
{
    struct u_hmap_queue_o_s *new;

    dbg_err_if (hmap == NULL);
    dbg_err_if (obj == NULL);
    dbg_err_if (data == NULL);

    if (*data == NULL) 
    {  /* no reference to queue entry */
        dbg_err_if ((new = _data_o_new(obj->key)) == NULL);
        TAILQ_INSERT_HEAD(&hmap->pcy.queue, new, next);
        *data = new;
    } else { /* have element in queue - move to head */
        TAILQ_REMOVE(&hmap->pcy.queue, *data, next);
        TAILQ_INSERT_HEAD(&hmap->pcy.queue, *data, next);
    }
    return 0;
    
err:
    return ~0;
}

static int _queue_push_count (u_hmap_t *hmap, struct u_hmap_o_s *obj, 
        struct u_hmap_queue_o_s **counts)
{
    struct u_hmap_queue_o_s *new, *t;
    int *count;

    dbg_err_if (hmap == NULL);
    dbg_err_if (obj == NULL);
    dbg_err_if (counts == NULL);

    if (*counts == NULL) /* no reference to queue entry */
    {  
        dbg_err_if ((new = _data_o_new(obj->key)) == NULL);
        TAILQ_INSERT_HEAD(&hmap->pcy.queue, new, next);
        *counts = TAILQ_FIRST(&hmap->pcy.queue);
        dbg_err_if ((count = (int *) u_zalloc(sizeof(int))) == NULL);
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
    return 0;
    
err:
    return ~0;
}

/**
 * \brief   Insert an object into the hmap
 * 
 * Insert a {\a key:\a val} pair into \a hmap. The object must be allocated
 * externally, but its ownership passes to \a hmap. Hence, the appropriate
 * object deallocation function should be set in the relative options field.
 * 
 * \param hmap      hmap object
 * \param key       key to be inserted 
 * \param val       value to be inserted
 * 
 * \return 0 on success, non-zero on failure
 */
int u_hmap_put (u_hmap_t *hmap, const char *key, void *val)
{
    struct u_hmap_o_s *obj, *new;
    struct u_hmap_e_s *x;
    int comp;

    dbg_err_if (hmap == NULL);
    dbg_err_if (key == NULL);
    dbg_err_if (val == NULL);

    dbg_err_if ((new = _o_new(key, val)) == NULL);
    x = &hmap->hmap[hmap->opts->f_hash(key, hmap->opts->max_size)];

    if (hmap->opts->policy != U_HMAP_PCY_NONE &&
            hmap->size >= hmap->opts->max_elems) 
    {
        dbg("Cache full - freeing according to policy %d", hmap->opts->policy);
        hmap->pcy.pop(hmap);
    }

    if (LIST_EMPTY(x)) 
    {
        LIST_INSERT_HEAD(x, new, next);
    } else {
        LIST_FOREACH(obj, x, next) 
        {
            if ((comp = hmap->opts->f_comp(key, obj->key)) == 0) 
            { 
                /* object already hmapd -> overwrite */
                LIST_INSERT_AFTER(obj, new, next);
                LIST_REMOVE(obj, next);
                _o_free(hmap, obj); 
                goto end;
            } else { 
                if (comp < 0) 
                {
                    LIST_INSERT_BEFORE(obj, new, next); 
                    break;
                } else if (!LIST_NEXT(obj, next)) {
                    LIST_INSERT_AFTER(obj, new, next);
                    break;
                }
            }
        }
    }
   
    hmap->size++;

end:
    if ((hmap->pcy.ops & U_HMAP_PCY_OP_PUT) == U_HMAP_PCY_OP_PUT)
        hmap->pcy.push(hmap, new, &new->pqe);

    return 0;

err:
    return ~0;
}

/**
 * \brief   Retrieve an object from the hmap
 * 
 * Retrieve object with given \a key from \a hmap. On success the requested
 * object is returned in \a val. The object is not removed from the hmap, so
 * ownership of the object is not returned to the user.
 * 
 * \param hmap      hmap object
 * \param key       key to be retrieved 
 * \param val       returned value
 * 
 * \return 0 on success, non-zero on failure
 */
int u_hmap_get (u_hmap_t *hmap, const char *key, void **val)
{
    struct u_hmap_o_s *obj = NULL;

    dbg_err_if (hmap == NULL);
    dbg_err_if (key == NULL);
    dbg_err_if (val == NULL);

    if (_get(hmap, key, &obj)) 
    {
        *val = NULL;
        return ~0;
    }
    dbg_err_if (obj == NULL);

    if ((hmap->pcy.ops & U_HMAP_PCY_OP_GET) == U_HMAP_PCY_OP_GET)
        hmap->pcy.push(hmap, obj, &obj->pqe);
        
    *val = obj->val;

    return 0;

err:
    return ~0;
}

/**
 * \brief   Perform an operation on all objects
 * 
 * Execute function \a f on all objects within \a hmap. These functions should 
 * return 0 on success, and take an object as a parameter.
 * 
 * \param hmap      hmap object
 * \param f         function    
 * 
 * \return 0 on success, non-zero on failure
 */
int u_hmap_foreach (u_hmap_t *hmap, int f(void *val))
{
    struct u_hmap_o_s *obj;
    size_t i;

    dbg_err_if (hmap == NULL);
    dbg_err_if (f == NULL);

    for (i = 0; i < hmap->opts->max_size; i++) 
    {
        LIST_FOREACH(obj, &hmap->hmap[i], next)
            dbg_err_if (f(obj->val));
    }

    return 0;

err:
    return ~0;
}

/**
 * \brief   Deallocate the hmap
 * 
 * Deallocate \a hmap along with options and all hmapd objects. Objects are
 * freed via free() by default or using the custom deallocation function passed
 * in the hmap options. 
 * 
 * \param hmap      hmap object
 * 
 * \return 0 on success, non-zero on failure
 */
int u_hmap_free (u_hmap_t *hmap)
{
    struct u_hmap_o_s *obj;
    struct u_hmap_queue_o_s *data;
    size_t i;

    dbg_return_if (hmap == NULL, ~0);

    /* free the hashhmap */
    for (i = 0; i < hmap->opts->max_size; i++) 
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
        _data_o_free(data);
    }

    u_free(hmap->opts);
    u_free(hmap);

    return 0;
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
 * \return 0 on success, non-zero on failure
 */
int u_hmap_opts_new (u_hmap_opts_t **opts)
{
    u_hmap_opts_t *o;

    dbg_err_if (opts == NULL);

    dbg_err_if ((o = (u_hmap_opts_t *) u_zalloc(sizeof(u_hmap_opts_t))) 
            == NULL);
    
    /* set defaults */
    o->max_size = U_HMAP_MAX_SIZE;
    o->max_elems = U_HMAP_MAX_ELEMS;
    o->policy = U_HMAP_PCY_NONE;
    o->f_hash = &_f_hash;
    o->f_comp = &_f_comp;
    o->f_free = &_f_free;
    o->f_str = NULL;
    
    *opts = o;
    
    return 0;
err:
    *opts = NULL;
    return ~0;
}

void u_hmap_opts_dbg (u_hmap_opts_t *opts)
{
    dbg_ifb (opts == NULL) return;

    dbg("[%u - %d,%d,%d,%x,%x,%x,%x]",
        sizeof(u_hmap_opts_t),
        opts->policy,
        opts->max_size,
        opts->max_elems,
        opts->f_hash,
        opts->f_comp,
        opts->f_free,
        opts->f_str);
}

static struct u_hmap_o_s *_o_new (const char *key, void *val)
{
    struct u_hmap_o_s *obj;

    dbg_return_if (key == NULL, NULL);
    dbg_return_if (val == NULL, NULL);

    dbg_return_if ((obj = (struct u_hmap_o_s *) 
                u_zalloc(sizeof(struct u_hmap_o_s))) == NULL, NULL);
    
    dbg_err_if ((obj->key = strdup(key)) == NULL);
    obj->val = val;
    obj->pqe = NULL;

    return obj;
 
err:
    u_free(obj);
    return NULL;
}

static void _o_free (u_hmap_t *hmap, struct u_hmap_o_s *obj)
{
    dbg_ifb (hmap == NULL) return;
    dbg_ifb (obj == NULL) return;
    
    hmap->opts->f_free(obj->val);  /* custom object deletion */
    u_free(obj->key);  /* strdup()ed key */
    u_free(obj); 
}

static struct u_hmap_queue_o_s *_data_o_new (const char *key)
{
    struct u_hmap_queue_o_s *data;

    dbg_return_if (key == NULL, NULL);

    dbg_return_if ((data = (struct u_hmap_queue_o_s *)
                u_zalloc(sizeof(struct u_hmap_queue_o_s))) == NULL,
            NULL);

    dbg_err_if ((data->key = strdup(key)) == NULL);
    data->o = NULL;
    
    return data;

err:
    u_free(data);
    return NULL;
}

static void _data_o_free (struct u_hmap_queue_o_s *data)
{
    dbg_ifb (data == NULL) return;

    u_free(data->o);
    u_free(data->key);
    u_free(data);
}

/**
 *      \}
 */

/**
 *  \}
 */
