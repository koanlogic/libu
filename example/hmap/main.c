/* $Id: main.c,v 1.3 2007/01/03 16:47:21 stewy Exp $ */

/* TODO public version of o_free () to free objects! */

#include <u/libu.h>

int facility = LOG_LOCAL0;

static int example_static(void);
static int example_dynamic_own_hmap(void);
static int example_dynamic_own_user(void);
static int example_no_overwrite(void);
static int example_hash_custom(void);

int main()
{
    con_err_if (example_static());
    con_err_if (example_dynamic_own_hmap());
    con_err_if (example_dynamic_own_user());
    con_err_if (example_no_overwrite());
    con_err_if (example_hash_custom());

    return 0;
err:    
    return ~0;
}

static int example_static()
{
    u_hmap_t *hmap = NULL;
    u_hmap_o_t *obj = NULL;
    int fibonacci[] = { 0, 1, 1, 2, 3, 5, 8, 13, 21 };

    con("example_static()");

    /* initialise hmap with no options - user owns data by default */
    con_err_if (u_hmap_new(NULL, &hmap));

    /* insert some sample elements */
    con_err_if (u_hmap_put(hmap, u_hmap_o_new("first", &fibonacci[0]), NULL));
    con_err_if (u_hmap_put(hmap, u_hmap_o_new("fifth", &fibonacci[4]), NULL)); 
    con_err_if (u_hmap_put(hmap, u_hmap_o_new("last", 
                &fibonacci[(sizeof(fibonacci)/sizeof(int))-1]), NULL));

    /* retrieve and print values to console */
    con_err_if (u_hmap_get(hmap, "last", &obj)); 
    con("hmap['%s'] = %d", (char *) obj->key, *((int *) obj->val));
    con_err_if (u_hmap_get(hmap, "fifth", &obj)); 
    con("hmap['%s'] = %d", (char *) obj->key, *((int *) obj->val)); 
    con_err_if (u_hmap_get(hmap, "first", &obj)); 
    con("hmap['%s'] = %d", (char *) obj->key, *((int *) obj->val));

    /* remove an element and replace it */
    con_err_if (u_hmap_del(hmap, "fifth", &obj)); 
    
    /* check that it has been deleted */
    con_err_if (u_hmap_get(hmap, "fifth", &obj) == 0); 

    /* replace with a new element and print it */
    con("decrementing hmap['%s']", "first");
    con_err_if (u_hmap_get(hmap, "first", &obj)); 
    int x = *((int *) obj->val);
    x--;
    con_err_if (u_hmap_put(hmap, u_hmap_o_new("first", &x), NULL));
    con_err_if (u_hmap_get(hmap, "first", &obj)); 
    con("hmap['%s'] = %d", (char *) obj->key, *((int *) obj->val));

    /* free hmap (options are freed automatically) */
    u_hmap_free(hmap);
    
    return 0;
err:
    return ~0;
}

static int example_dynamic_own_hmap()
{
    u_hmap_opts_t *opts = NULL;
    u_hmap_t *hmap = NULL;
    u_hmap_o_t *obj = NULL;

    con("example_dynamic_own_hmap()");

    /* initialise options and hmap */
    con_err_if (u_hmap_opts_new(&opts));
    /* hmap owns both keys and data */
    opts->options |= U_HMAP_OPTS_OWNSDATA;
    con_err_if (u_hmap_new(opts, &hmap));

    /* insert some sample elements */
    con_err_if (u_hmap_put(hmap, u_hmap_o_new(strdup("english"), 
                    strdup("Hello world!")), NULL));
    con_err_if (u_hmap_put(hmap, u_hmap_o_new(strdup("italian"), 
                    strdup("Ciao mondo!")), NULL));
    con_err_if (u_hmap_put(hmap, u_hmap_o_new(strdup("german"), 
                    strdup("Hallo Welt!")), NULL));

    /* retrieve and print values to console */
    con_err_if (u_hmap_get(hmap, "italian", &obj)); 
    con("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);
    con_err_if (u_hmap_get(hmap, "german", &obj)); 
    con("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);
    con_err_if (u_hmap_get(hmap, "english", &obj)); 
    con("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);

    /* remove an element and replace it */
    con_err_if (u_hmap_del(hmap, "german", NULL)); 

    /* check that it has been deleted */
    con_err_if (u_hmap_get(hmap, "german", &obj) == 0); 

    /* replace with a new element and print it */
    con_err_if (u_hmap_put(hmap, u_hmap_o_new(strdup("german"), 
                    strdup("Auf Wiedersehen!")), NULL));
    con_err_if (u_hmap_get(hmap, "german", &obj)); 
    con("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);

    /* free hmap (options and elements are freed automatically) */
    u_hmap_free(hmap);

    return 0;
err:
    return ~0;
}

static int example_dynamic_own_user()
{
    u_hmap_t *hmap = NULL;
    u_hmap_o_t *obj = NULL;

#define OBJ_FREE(obj) \
    if (obj) { \
        u_free(obj->key); \
        u_free(obj->val); \
        obj = NULL; \
    }
    
    con("example_dynamic_own_user()");

    /* hmap owns both keys and data */
    con_err_if (u_hmap_new(NULL, &hmap));

    /* insert some sample elements */
    con_err_if (u_hmap_put(hmap, u_hmap_o_new(strdup("english"), 
                    strdup("Hello world!")), &obj));
    con_err_if (u_hmap_put(hmap, u_hmap_o_new(strdup("italian"), 
                    strdup("Ciao mondo!")), &obj));
    con_err_if (u_hmap_put(hmap, u_hmap_o_new(strdup("german"), 
                    strdup("Hallo Welt!")), &obj));

    /* retrieve and print values to console */
    con_err_if (u_hmap_get(hmap, "italian", &obj)); 
    con("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);
    con_err_if (u_hmap_get(hmap, "german", &obj)); 
    con("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);
    con_err_if (u_hmap_get(hmap, "english", &obj)); 
    con("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);

    /* remove an element and replace it */
    con_err_if (u_hmap_del(hmap, "german", &obj)); 
    OBJ_FREE(obj);

    /* check that it has been deleted */
    con_err_if (u_hmap_get(hmap, "german", &obj) == 0); 

    /* replace with a new element and print it */
    con_err_if (u_hmap_put(hmap, u_hmap_o_new(strdup("german"), 
                    strdup("Auf Wiedersehen!")), &obj));
    OBJ_FREE(obj);
    con_err_if (u_hmap_put(hmap, u_hmap_o_new(strdup("german"), 
                    strdup("Auf Wiedersehen2!")), &obj));
    OBJ_FREE(obj);
    con_err_if (u_hmap_get(hmap, "german", &obj)); 
    con("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);

    u_hmap_del(hmap, "italian", &obj);
    OBJ_FREE(obj);
    u_hmap_del(hmap, "german", &obj);
    OBJ_FREE(obj);
    u_hmap_del(hmap, "english", &obj);
    OBJ_FREE(obj);

    /* free hmap (options and elements are freed automatically) */
    u_hmap_free(hmap);

    return 0;
err:
    return ~0;
}

static int example_no_overwrite()
{
#define MAP_INSERT(hmap, key, val, obj) \
    switch (u_hmap_put(hmap, u_hmap_o_new(key, val), &obj)) { \
        case U_HMAP_ERR_NONE: \
            break; \
        case U_HMAP_ERR_EXISTS: \
            u_hmap_o_free(obj); \
            break; \
        case U_HMAP_ERR_FAIL: \
            goto err; \
    }

    u_hmap_opts_t *opts = NULL;
    u_hmap_t *hmap = NULL;
    u_hmap_o_t *obj = NULL;

    con("example_no_overwrite()");

    /* initialise options and hmap */
    con_err_if (u_hmap_opts_new(&opts));
    /* hmap owns both keys and data */
    opts->options |= U_HMAP_OPTS_NO_OVERWRITE;
    con_err_if (u_hmap_new(opts, &hmap));

    /* insert some sample elements with the same key */
    MAP_INSERT(hmap, "A", "A1", obj);
    MAP_INSERT(hmap, "A", "A2", obj);
    MAP_INSERT(hmap, "A", "A3", obj);

    /* retrieve and print values to console */
    con_err_if (u_hmap_get(hmap, "A", &obj)); 
    con("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);

    /* free hmap (options and elements are freed automatically) */
    u_hmap_free(hmap);

    return 0;
err:
    return ~0;
#undef MAP_INSERT
}

#if 0
static int example_no_ordering()
{
    u_hmap_opts_t *opts = NULL;
    u_hmap_t *hmap = NULL;
    u_hmap_o_t *obj = NULL;
    
    con("example_no_ordering"); 

    con_err_if (u_hmap_opts_new(&opts));
    opts->max_size = 3;
    opts->options |= U_HMAP_OPTS_NO_ORDERING;
    con_err_if (u_hmap_new(opts, &hmap));

    con_err_if (u_hmap_put(hmap, u_hmap_o_new("F", "1"), NULL));
    con_err_if (u_hmap_put(hmap, u_hmap_o_new("E", "2"), NULL));
    con_err_if (u_hmap_put(hmap, u_hmap_o_new("B", "3"), NULL));
    con_err_if (u_hmap_put(hmap, u_hmap_o_new("B", "3.1"), NULL));
    con_err_if (u_hmap_put(hmap, u_hmap_o_new("B", "3.2"), NULL));
    con_err_if (u_hmap_put(hmap, u_hmap_o_new("A", "4"), NULL));
    con_err_if (u_hmap_put(hmap, u_hmap_o_new("D", "5"), NULL));
    con_err_if (u_hmap_put(hmap, u_hmap_o_new("C", "6"), NULL));

//    con_err_if (u_hmap_get(hmap, "A", &obj)); 
//    con("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);
    u_hmap_dbg(hmap);

    u_hmap_free(hmap);

    return 0;
err:
    return 0;
}
#endif

static int example_hash_custom()
{
#define MAP_INSERT(hmap, k, v) \
    con_err_if ((o[k-1] = _sample_obj(k, v)) == NULL); \
    con_err_if (u_hmap_put(hmap, o[k-1], NULL));

    static size_t _sample_hash(void *key, size_t size)
    {
        return (*((int *) key) % size);
    };

    /* Allocate (key, value) pair dynamically */
    static u_hmap_o_t *_sample_obj(int key, const char *val)
    {
        u_hmap_o_t *new = NULL;

        int *k = NULL;
        char *v = NULL;
        
        k = (int *) malloc(sizeof(int));
        con_err_if (k == NULL);
        *k = key;

        v = strdup(val);
        con_err_if (v == NULL);
        
        new = u_hmap_o_new(k, v);
        con_err_if (new == NULL);
        
        return new;

    err:
        u_free(k);
        u_free(v);
        
        return NULL;
    };

    u_hmap_opts_t *opts = NULL;
    u_hmap_t *hmap = NULL;
    u_hmap_o_t *o[3], *obj = NULL;
    
    con("example_hash_custom()"); 

    con_err_if (u_hmap_opts_new(&opts));
    opts->options |= U_HMAP_OPTS_OWNSDATA | U_HMAP_OPTS_HASH_STRONG;
    opts->f_hash = &_sample_hash;

    con_err_if (u_hmap_new(opts, &hmap));

    MAP_INSERT(hmap, 1, "one");
    MAP_INSERT(hmap, 2, "two");
    MAP_INSERT(hmap, 3, "three");

    con_err_if (u_hmap_get(hmap, o[0]->key, &obj)); 
    con("hmap['%d'] = %s", *((int *) obj->key), (char *) obj->val);
    con_err_if (u_hmap_get(hmap, o[1]->key, &obj)); 
    con("hmap['%d'] = %s", *((int *) obj->key), (char *) obj->val);
    con_err_if (u_hmap_get(hmap, o[2]->key, &obj)); 
    con("hmap['%d'] = %s", *((int *) obj->key), (char *) obj->val);

    u_hmap_free(hmap);

    return 0;
err:
    return 0;
#undef MAP_INSERT
}
