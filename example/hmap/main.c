/* $Id: main.c,v 1.5 2007/01/16 20:38:33 stewy Exp $ */

/* TODO public version of o_free () to free objects! */

#include <string.h>

#include <u/libu.h>

int facility = LOG_LOCAL0;

static int example_static(void);
static int example_dynamic_own_hmap(void);
static int example_dynamic_own_user(void);
static int example_no_overwrite(void);
static int example_types_custom(void);

int main()
{
    con_err_if (example_static());
    con_err_if (example_dynamic_own_hmap());
    con_err_if (example_dynamic_own_user());
    con_err_if (example_no_overwrite());
    con_err_if (example_types_custom());

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
    u_hmap_o_free(obj);
    
    /* check that it has been deleted */
    con_err_if (u_hmap_get(hmap, "fifth", &obj) == 0); 

    /* delete the other two elements */
    con_err_if (u_hmap_del(hmap, "last", &obj)); 
    u_hmap_o_free(obj);
    con_err_if (u_hmap_del(hmap, "first", &obj)); 
    u_hmap_o_free(obj);

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
    con_err_if (u_hmap_put(hmap, u_hmap_o_new((void *) u_strdup("english"), 
                    (void *) u_strdup("Hello world!")), NULL));
    con_err_if (u_hmap_put(hmap, u_hmap_o_new((void *) u_strdup("italian"), 
                    (void *) u_strdup("Ciao mondo!")), NULL));
    con_err_if (u_hmap_put(hmap, u_hmap_o_new((void *) u_strdup("german"), 
                    (void *) u_strdup("Hallo Welt!")), NULL));

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
    con_err_if (u_hmap_put(hmap, u_hmap_o_new((void *) u_strdup("german"), 
                    (void *) u_strdup("Auf Wiedersehen!")), NULL));
    con_err_if (u_hmap_get(hmap, "german", &obj)); 
    con("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);

    /* free hmap (options and elements are freed automatically) */
    u_hmap_dbg(hmap);
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
        u_hmap_o_free(obj); \
        obj = NULL; \
    }
    
    con("example_dynamic_own_user()");

    /* hmap owns both keys and data */
    con_err_if (u_hmap_new(NULL, &hmap));

    /* insert some sample elements */
    con_err_if (u_hmap_put(hmap, u_hmap_o_new((void *) u_strdup("english"), 
                    (void *) u_strdup("Hello world!")), &obj));
    con_err_if (u_hmap_put(hmap, u_hmap_o_new((void *) u_strdup("italian"), 
                    (void *) u_strdup("Ciao mondo!")), &obj));
    con_err_if (u_hmap_put(hmap, u_hmap_o_new((void *) u_strdup("german"), 
                    (void *) u_strdup("Hallo Welt!")), &obj));

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
    con_err_if (u_hmap_put(hmap, u_hmap_o_new((void *) u_strdup("german"), 
                    (void *) u_strdup("Auf Wiedersehen!")), NULL));
    con_err_if (u_hmap_put(hmap, u_hmap_o_new((void *) u_strdup("german"), 
                    (void *) u_strdup("Auf Wiedersehen2!")), &obj));
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
    con_err_if (u_hmap_del(hmap, "A", &obj)); 
    u_hmap_o_free(obj); 

    /* free hmap (options and elements are freed automatically) */
    u_hmap_free(hmap);

    return 0;
err:
    return ~0;
#undef MAP_INSERT
}


static int example_types_custom()
{
#define MAP_INSERT(hmap, k, v) \
    con_err_if ((obj = _sample_obj(k, v)) == NULL); \
    con_err_if (u_hmap_put(hmap, obj, NULL));

    size_t _sample_hash(void *key, size_t size)
    {
        return (*((int *) key) % size);
    };

    int _sample_comp(void *key1, void *key2)
    {
        int k1 = *((int *) key1),
            k2 = *((int *) key2);
        
        return k1 < k2 ? -1 : ((k1 > k2)? 1 : 0);
    };

    u_string_t *_sample_str(u_hmap_o_t *obj)
    {
        enum { MAX_OBJ_STR = 256 };
        char buf[MAX_OBJ_STR];
        u_string_t *s = NULL;

        int key = *((int *) obj->key);
        char *val = (char *) obj->val;

        con_err_if (u_snprintf(buf, MAX_OBJ_STR, "[%d:%s]", key, val));
        con_err_if (u_string_create(buf, strlen(buf)+1, &s));

        return s;

    err:
        return NULL;
    };

    /* Allocate (key, value) pair dynamically */
    u_hmap_o_t *_sample_obj(int key, const char *val)
    {
        u_hmap_o_t *new = NULL;

        int *k = NULL;
        char *v = NULL;
        
        k = (int *) malloc(sizeof(int));
        con_err_if (k == NULL);
        *k = key;

        v = u_strdup(val);
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
    u_hmap_o_t *obj = NULL; 
    
    con("example_types_custom()"); 

    con_err_if (u_hmap_opts_new(&opts));
    opts->options |= U_HMAP_OPTS_OWNSDATA | U_HMAP_OPTS_HASH_STRONG;
    opts->max_size = 3;
    opts->f_hash = &_sample_hash;
    opts->f_comp = &_sample_comp;
    opts->f_str = &_sample_str;

    con_err_if (u_hmap_new(opts, &hmap));

    MAP_INSERT(hmap, 2, "two");
    MAP_INSERT(hmap, 1, "one");
    MAP_INSERT(hmap, 4, "four");
    MAP_INSERT(hmap, 7, "seven");
    MAP_INSERT(hmap, 4, "four2");
    MAP_INSERT(hmap, 3, "three");
    MAP_INSERT(hmap, 6, "six");
    MAP_INSERT(hmap, 1, "one2");
    MAP_INSERT(hmap, 5, "five");

    int x = 1;
    con_err_if (u_hmap_get(hmap, &x, &obj)); 
    con("hmap['%d'] = %s", *((int *) obj->key), (char *) obj->val);
    x++;
    con_err_if (u_hmap_get(hmap, &x, &obj)); 
    con("hmap['%d'] = %s", *((int *) obj->key), (char *) obj->val);
    x++;
    con_err_if (u_hmap_get(hmap, &x, &obj)); 
    con("hmap['%d'] = %s", *((int *) obj->key), (char *) obj->val);
    
    u_hmap_dbg(hmap);
    u_hmap_free(hmap);

    return 0;
err:
    return ~0;
#undef MAP_INSERT
}
