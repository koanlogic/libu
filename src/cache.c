/* $Id: cache.c,v 1.1 2006/01/04 15:32:45 stewy Exp $ */

#include <u/cache.h>
#include <u/libu.h>
#include <assert.h>

/**
 *  \defgroup cache Cache 
 *  \{
 */


/* default limits handled by policies */
#define U_CACHE_MAX_SIZE      512
#define U_CACHE_MAX_ELEMS     U_CACHE_MAX_SIZE


/* policy queue object */
struct u_cache_queue_o_s {
    char               *key;
    void               *o;
    TAILQ_ENTRY(
        u_cache_queue_o_s) next;
};

/* cache object abstraction */
struct u_cache_o_s {
    char                       *key;
    void                       *val;
    LIST_ENTRY(
        u_cache_o_s)            next;
    struct u_cache_queue_o_s    *pqe; 
};

/* cache policy representation */
struct u_cache_pcy_s {
        int                   (*pop)(struct u_cache_s *cache); 
        int                   (*push)(struct u_cache_s *cache, 
                                  struct u_cache_o_s *obj,
                                  struct u_cache_queue_o_s **data); 
        TAILQ_HEAD(
            u_cache_queue_h_s, 
            u_cache_queue_o_s)   queue;
        enum {
            U_CACHE_PCY_OP_PUT = 0x1,
            U_CACHE_PCY_OP_GET = 0x2
        } ops;
};

/* cache representation */
struct u_cache_s {
    u_cache_opts_t     *opts;
    size_t              size;
    LIST_HEAD(
        u_cache_e_s, 
        u_cache_o_s)    *map;
   
    struct u_cache_pcy_s    pcy;
};


static int _u_cache_get (u_cache_t *cache, const char *key, 
                        struct u_cache_o_s **o);

static int _u_cache_opts_check (u_cache_opts_t *opts);
static int _u_cache_pcy_setup (u_cache_t *cache);

static struct u_cache_o_s *_u_cache_o_new (const char *key, void *val);
static void _u_cache_o_free (u_cache_t *cache, struct u_cache_o_s *obj);

static struct u_cache_queue_o_s *_u_cache_data_o_new();
static void _u_cache_data_o_free (struct u_cache_queue_o_s *s);

static size_t _u_cache_f_hash (const const char *key, size_t buckets);
static int _u_cache_f_comp (const char *k1, const char *k2);
static void _u_cache_f_free (void *val);

static int _u_cache_queue_push (u_cache_t *cache, struct u_cache_o_s *obj, 
                               struct u_cache_queue_o_s **data);
static int _u_cache_queue_push_count (u_cache_t *cache, struct u_cache_o_s *obj, 
                                     struct u_cache_queue_o_s **counts);
static int _u_cache_queue_pop_front (u_cache_t *cache);
static int _u_cache_queue_pop_back (u_cache_t *cache);


static size_t _u_cache_f_hash (const const char *key, size_t buckets)
{
    size_t h = 0;

    assert(key && buckets > 0);

    while (*key)
    {
        h += *key++;
        h += (h << 10);
        h ^= (h >> 6);
    }

    h += (h << 3);
    h ^= (h >> 11);

    return (h + (h << 15)) % buckets;
}

static int _u_cache_f_comp (const char *k1, const char *k2) 
{
    assert(k1 && k2);

    return strcmp(k1, k2);               /* user should check length */
}

static void _u_cache_f_free (void *val)
{
    dbg_return_if (val == NULL, );

    u_free(val); 
}

static int _u_cache_opts_check (u_cache_opts_t *opts)
{
    dbg_err_if (opts == NULL);

    dbg_err_if (opts->max_size == 0);
    dbg_err_if (opts->max_elems == 0);
    dbg_err_if (opts->policy < U_CACHE_PCY_NONE || opts->policy > U_CACHE_PCY_LFU);
    dbg_err_if (opts->f_hash == NULL);
    dbg_err_if (opts->f_comp == NULL);
    dbg_err_if (opts->f_free == NULL);
     
    return 0;

err:
    return ~0;
}

static int _u_cache_pcy_setup (u_cache_t *cache) 
{
    dbg_return_if (cache == NULL, ~0);

    switch (cache->opts->policy) {
        case U_CACHE_PCY_NONE:
            cache->pcy.pop = NULL;
            cache->pcy.push = NULL;
            cache->pcy.ops = 0;
            break;
        case U_CACHE_PCY_LRU:
            cache->pcy.push = _u_cache_queue_push;
            cache->pcy.pop = _u_cache_queue_pop_back;
            cache->pcy.ops = U_CACHE_PCY_OP_PUT | U_CACHE_PCY_OP_GET;
            break;
        case U_CACHE_PCY_FIFO:
            cache->pcy.push = _u_cache_queue_push;
            cache->pcy.pop = _u_cache_queue_pop_back;
            cache->pcy.ops = U_CACHE_PCY_OP_PUT;
            break;
        case U_CACHE_PCY_LFU:
            cache->pcy.push = _u_cache_queue_push_count;
            cache->pcy.pop = _u_cache_queue_pop_front;
            cache->pcy.ops = U_CACHE_PCY_OP_PUT | U_CACHE_PCY_OP_GET;
            break;
        default:
            dbg("Invalid policy: %d", cache->opts->policy);
            return ~0;
    }
    return 0;
}

/**
 * \brief   Create a new cache 
 * 
 * Create a new cache object and save its pointer to \a *cache. 
 * The call may fail on memory allocation problems or if the options are
 * manipulated incorrectly.
 * 
 * \param opts      options to be passed to the cache
 * \param cache     on success contains the cache options object
 * 
 * \return 0 on success, non-zero on failure
 */
int u_cache_new (u_cache_opts_t *opts, u_cache_t **cache)
{
    int i;
    u_cache_t *c = NULL;

    dbg_return_if (opts == NULL, ~0);
    dbg_return_if (cache == NULL, ~0);
   
    dbg_return_if ((c = (u_cache_t *) u_zalloc(sizeof(u_cache_t))) == NULL, ~0);
    
    if (opts == NULL)
        dbg_err_if (u_cache_opts_new(&c->opts));
    else { 
        c->opts = opts;
        dbg_err_if (_u_cache_opts_check(c->opts));
    }
    dbg_err_if (_u_cache_pcy_setup(c));

    c->size = 0;
    dbg_err_if ((c->map = (struct u_cache_e_s *) 
                          u_zalloc(sizeof(struct u_cache_e_s) * 
                          c->opts->max_size)) == NULL);

    /* initialise entries */
    for (i = 0; i < c->opts->max_size; i++)
        LIST_INIT(&c->map[i]);

    TAILQ_INIT(&c->pcy.queue);

    *cache = c;

    return 0;

err:
    u_free(c);
    *cache = NULL;    
    return ~0;
}

void u_cache_dbg (u_cache_t *cache)
{
    enum { MAX_LINE = 255 };
    u_string_t *s = NULL, *st = NULL;
    struct u_cache_o_s *obj;
    int i;

    dbg_return_if (cache == NULL, );

    dbg ("<cache>");
    for (i = 0; i < cache->opts->max_size; i++) {
        dbg_return_if ((u_string_create("", 1, &s)), );
        dbg_err_if (u_string_clear(s));
        dbg_err_if (u_string_append(s, "|", 1));
        LIST_FOREACH(obj, &cache->map[i], next) {
            dbg_err_if (u_string_append(s, "[", 1));
            dbg_err_if (u_string_append(s, obj->key, strlen(obj->key)));
            dbg_err_if (u_string_append(s, ":", 1));
            if (cache->opts->f_str == NULL)
                dbg_err_if (u_string_append(s, "*", 1));
            else {
                st = cache->opts->f_str(obj->val);
                dbg_err_if (u_string_append(s, u_string_c(st),
                            u_string_len(st)-1));
            }
            dbg_err_if (u_string_append(s, "]", 1));
        } 
        dbg_err_if (u_string_append(s, "|", 1));
        dbg(u_string_c(s));
        dbg_return_if (u_string_free(s), );
    }
    dbg("</cache>");
    return;

err:
    u_string_free(s);
    return;   
}

/**
 * \brief   Delete an object from the cache
 * 
 * Delete object with given \a key from \a cache.
 * 
 * \param cache     cache object
 * \param key       key of object to be deleted 
 * 
 * \return 0 on success, non-zero on failure
 */
int u_cache_del (u_cache_t *cache, const char *key) 
{
    struct u_cache_o_s *obj = NULL;

    dbg_err_if (cache == NULL);
    dbg_err_if (key == NULL);

    if (_u_cache_get(cache, key, &obj))
        return ~0;

    dbg_err_if (obj == NULL);
    _u_cache_o_free(cache, obj);
    LIST_REMOVE(obj, next);
    return 0;
    
err:
    return ~0;
}

static int _u_cache_get (u_cache_t *cache, const char *key, 
                       struct u_cache_o_s **o)
{
    struct u_cache_o_s *obj;
    struct u_cache_e_s *x;
    int comp;

    dbg_err_if (cache == NULL);
    dbg_err_if (key == NULL);
    dbg_err_if (o == NULL);

    x = &cache->map[cache->opts->f_hash(key, cache->opts->max_size)];

    LIST_FOREACH(obj, x, next) {
        if ((comp = cache->opts->f_comp(key, obj->key)) == 0) {   /* object found */
            *o = obj;
            return 0;
        } else if (comp < 0) {  /* cannot be in list (ordered) */
            *o = NULL;
            break;
        }
    }

err: 
    return ~0;
}

void u_cache_pcy_dbg (u_cache_t *cache)
{
    struct u_cache_queue_o_s *data;
    u_string_t *s = NULL;

    dbg_return_if (cache == NULL, );

    dbg_return_if ((u_string_create("", 1, &s)), );
    dbg_err_if (u_string_clear(s));
    dbg_err_if (u_string_append(s, "Policy: [", strlen("Policy: [")));
    TAILQ_FOREACH(data, &cache->pcy.queue, next) {
        dbg_err_if (u_string_append(s, "(", 1));
        dbg_err_if (u_string_append(s, data->key, strlen(data->key)));
        dbg_err_if (u_string_append(s, ")", 1));
    }
    dbg_err_if (u_string_append(s, "]", 1));
    dbg(u_string_c(s));
    dbg_ifb (u_string_free(s));
    return;
    
 err:
    u_string_free(s);
    return;
}

static int _u_cache_queue_pop_front (u_cache_t *cache)
{
    struct u_cache_queue_o_s *first;

    dbg_err_if (cache == NULL);

    dbg_err_if ((first = TAILQ_FIRST(&cache->pcy.queue)) == NULL);
    dbg_err_if (u_cache_del(cache, first->key));
    _u_cache_data_o_free(first);
    TAILQ_REMOVE(&cache->pcy.queue, first, next);

    return 0;

err:
    return ~0;
}

static int _u_cache_queue_pop_back (u_cache_t *cache)
{
    struct u_cache_queue_o_s *last;

    dbg_err_if (cache == NULL);

    dbg_err_if ((last = TAILQ_LAST(&cache->pcy.queue, u_cache_queue_h_s)) == NULL);
    dbg_err_if (u_cache_del(cache, last->key));
    _u_cache_data_o_free(last);
    TAILQ_REMOVE(&cache->pcy.queue, last, next);
    
    return 0;

err: 
    return ~0;
}

static int _u_cache_queue_push (u_cache_t *cache, struct u_cache_o_s *obj, 
                                struct u_cache_queue_o_s **data)
{
    struct u_cache_queue_o_s *new;

    dbg_err_if (cache == NULL);
    dbg_err_if (obj == NULL);
    dbg_err_if (data == NULL);

    if (*data == NULL) {  /* no reference to queue entry */
        dbg_err_if ((new = _u_cache_data_o_new(obj->key)) == NULL);
        TAILQ_INSERT_HEAD(&cache->pcy.queue, new, next);
        *data = new;
    } else {  /* have element in queue - move to head */
        TAILQ_REMOVE(&cache->pcy.queue, *data, next);
        TAILQ_INSERT_HEAD(&cache->pcy.queue, *data, next);
    }
    return 0;
    
err:
    return ~0;
}

static int _u_cache_queue_push_count (u_cache_t *cache, struct u_cache_o_s *obj, 
                                      struct u_cache_queue_o_s **counts)
{
    struct u_cache_queue_o_s *new, *t, *tn;
    int *count, *c;

    dbg_err_if (cache == NULL);
    dbg_err_if (obj == NULL);
    dbg_err_if (counts == NULL);

    if (*counts == NULL) {  /* no reference to queue entry */
        dbg_err_if ((new = _u_cache_data_o_new(obj->key)) == NULL);
        TAILQ_INSERT_HEAD(&cache->pcy.queue, new, next);
        *counts = TAILQ_FIRST(&cache->pcy.queue);
        dbg_err_if ((count = (int *) u_zalloc(sizeof(int))) == NULL);
        new->o = (void *) count;
        *counts = new;
    } else {  /* have element in queue - move to head */
        count = (int *) (*counts)->o;
        memset((void *) count, (*count)++, sizeof(int));
        if (t = TAILQ_NEXT(*counts, next)) {
            for (; t && ((*count) > *((int *) t->o)); t = TAILQ_NEXT(t, next))
                ;
            TAILQ_REMOVE(&cache->pcy.queue, *counts, next);
            if (t)
                TAILQ_INSERT_BEFORE(t, *counts, next);
            else
                TAILQ_INSERT_TAIL(&cache->pcy.queue, *counts, next);
        }
    }
    return 0;
    
err:
    return ~0;
}

/**
 * \brief   Insert an object into the cache
 * 
 * Insert a {\a key:\a val} pair into \a cache. The object must be allocated
 * externally, but its ownership passes to \a cache. Hence, the appropriate
 * object deallocation function should be set in the relative options field.
 * 
 * \param cache     cache object
 * \param key       key to be inserted 
 * \param val       value to be inserted
 * 
 * \return 0 on success, non-zero on failure
 */
int u_cache_put (u_cache_t *cache, const char *key, void *val)
{
    struct u_cache_o_s *obj, *new;
    struct u_cache_e_s *x;
    int comp;

    dbg_err_if (cache == NULL);
    dbg_err_if (key == NULL);
    dbg_err_if (val == NULL);

    dbg_err_if ((new = _u_cache_o_new(key, val)) == NULL);
    x = &cache->map[cache->opts->f_hash(key, cache->opts->max_size)];

    if (cache->opts->policy != U_CACHE_PCY_NONE && 
            cache->size >= cache->opts->max_elems) {
        dbg("Cache full - freeing according to policy %d", cache->opts->policy);
        cache->pcy.pop(cache);
    }

    if (LIST_EMPTY(x)) {
        LIST_INSERT_HEAD(x, new, next);
    } else {
        LIST_FOREACH(obj, x, next) {
            if ((comp = cache->opts->f_comp(key, obj->key)) == 0) { 
                /* object already cached -> overwrite */
                LIST_INSERT_AFTER(obj, new, next);
                LIST_REMOVE(obj, next);
                _u_cache_o_free(cache, obj); 
                goto end;
            } else { 
                if (comp < 0) {
                    LIST_INSERT_BEFORE(obj, new, next); 
                    break;
                } else if (!LIST_NEXT(obj, next)) {
                    LIST_INSERT_AFTER(obj, new, next);
                    break;
                }
            }
        }
    }
   
    cache->size++;
end:
    if ((cache->pcy.ops & U_CACHE_PCY_OP_PUT) == U_CACHE_PCY_OP_PUT)
        cache->pcy.push(cache, new, &new->pqe);
    return 0;

err:
    return ~0;
}

/**
 * \brief   Retrieve an object from the cache
 * 
 * Retrieve object with given \a key from \a cache. On success the requested
 * object is returned in \a val. The object is not removed from the cache, so
 * ownership of the object is not returned to the user.
 * 
 * \param cache     cache object
 * \param key       key to be retrieved 
 * \param val       returned value
 * 
 * \return 0 on success, non-zero on failure
 */
int u_cache_get (u_cache_t *cache, const char *key, void **val)
{
    struct u_cache_o_s *obj = NULL;

    dbg_err_if (cache == NULL);
    dbg_err_if (key == NULL);
    dbg_err_if (val == NULL);

    if (_u_cache_get(cache, key, &obj)) {
        *val = NULL;
        return ~0;
    }
    dbg_err_if (obj == NULL);

    if ((cache->pcy.ops & U_CACHE_PCY_OP_GET) == U_CACHE_PCY_OP_GET)
        cache->pcy.push(cache, obj, &obj->pqe);
        
    *val = obj->val;

    return 0;

err:
    return ~0;
}

/**
 * \brief   Perform an operation on all objects
 * 
 * Execute function \a f on all objects within \a cache. These functions should 
 * return 0 on success, and take an object as a parameter.
 * 
 * \param cache     cache object
 * \param f         function    
 * 
 * \return 0 on success, non-zero on failure
 */
int u_cache_foreach (u_cache_t *cache, int f(void *val))
{
    struct u_cache_o_s *obj;
    int i;

    dbg_err_if (cache == NULL);
    dbg_err_if (f == NULL);

    for (i = 0; i < cache->opts->max_size; i++) {
        LIST_FOREACH(obj, &cache->map[i], next)
            dbg_err_if (f(obj->val));
    }

    return 0;

err:
    return ~0;
}

/**
 * \brief   Deallocate the cache
 * 
 * Deallocate \a cache along with options and all cached objects. Objects are
 * freed via free() by default or using the custom deallocation function passed
 * in the cache options. 
 * 
 * \param cache     cache object
 * 
 * \return 0 on success, non-zero on failure
 */
int u_cache_free (u_cache_t *cache)
{
    struct u_cache_o_s *obj;
    struct u_cache_queue_o_s *data;
    int i;

    dbg_return_if (cache == NULL, ~0);

    /* free the hashmap */
    for (i = 0; i < cache->opts->max_size; i++) {
        while ((obj = LIST_FIRST(&cache->map[i])) != NULL) {
            _u_cache_o_free(cache, obj);
            LIST_REMOVE(obj, next);
        }
    }
    u_free(cache->map);

    /* free the policy queue */
    while ((data = TAILQ_FIRST(&cache->pcy.queue)) != NULL) {
        _u_cache_data_o_free(data);
        TAILQ_REMOVE(&cache->pcy.queue, data, next);
    }
    u_free(cache->opts);
    u_free(cache);

    return 0;
}

/**
 * \brief   Create new cache options
 * 
 * Create a new cache options object and save its pointer to \a *opts. 
 * The fields within the object can be manipulated publicly according to the
 * description in the header file.
 * 
 * \param opts  on success contains the cache options object
 * 
 * \return 0 on success, non-zero on failure
 */
int u_cache_opts_new (u_cache_opts_t **opts)
{
    u_cache_opts_t *o;

    dbg_err_if (opts == NULL);

    dbg_err_if ((o = (u_cache_opts_t *) u_zalloc(sizeof(u_cache_opts_t))) == NULL);
    
    /* set defaults */
    o->max_size = U_CACHE_MAX_SIZE;
    o->max_elems = U_CACHE_MAX_ELEMS;
    o->policy = U_CACHE_PCY_LRU;
    o->f_hash = &_u_cache_f_hash;
    o->f_comp = &_u_cache_f_comp;
    o->f_free = &_u_cache_f_free;
    o->f_str = NULL;
    
    *opts = o;
    
    return 0;
err:
    *opts = NULL;
    return ~0;
}

void u_cache_opts_dbg (u_cache_opts_t *opts)
{
    dbg_return_if (opts == NULL, );

    dbg("[%u - %d,%d,%d,%x,%x,%x,%x]",
        sizeof(u_cache_opts_t),
        opts->policy,
        opts->max_size,
        opts->max_elems,
        opts->f_hash,
        opts->f_comp,
        opts->f_free,
        opts->f_str);
}

static struct u_cache_o_s *_u_cache_o_new (const char *key, void *val)
{
    struct u_cache_o_s *obj;

    dbg_return_if (key == NULL, NULL);
    dbg_return_if (val == NULL, NULL);

    dbg_return_if ((obj = (struct u_cache_o_s *) 
                       u_zalloc(sizeof(struct u_cache_o_s))) == NULL, NULL);
    
    dbg_err_if ((obj->key = strdup(key)) == NULL);
    obj->val = val;
    obj->pqe = NULL;

    return obj;
 
err:
    u_free(obj);
    return NULL;
}

static void _u_cache_o_free (u_cache_t *cache, struct u_cache_o_s *obj)
{
    dbg_return_if (cache == NULL, );
    dbg_return_if (obj == NULL, );
    
    cache->opts->f_free(obj->val);  /* custom object deletion */
    u_free(obj->key);  /* strdup()ed key */
    u_free(obj); 
}

static struct u_cache_queue_o_s *_u_cache_data_o_new(const char *key)
{
    struct u_cache_queue_o_s *data;

    dbg_return_if (key == NULL, NULL);

    dbg_return_if ((data = (struct u_cache_queue_o_s *)
                        u_zalloc(sizeof(struct u_cache_queue_o_s))) == NULL, NULL);

    dbg_err_if ((data->key = strdup(key)) == NULL);
    data->o = NULL;
    
    return data;

err:
    u_free(data);
    return NULL;
}

static void _u_cache_data_o_free (struct u_cache_queue_o_s *data)
{
    dbg_return_if (data == NULL, );

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
