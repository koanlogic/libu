#include <u/libu.h>
#include <string.h>

U_TEST_SUITE(hmap);

static size_t __sample_hash (void *key, size_t size);
static int __sample_comp(void *key1, void *key2);
static u_string_t *__sample_str(u_hmap_o_t *obj);
static u_hmap_o_t *__sample_obj(int key, const char *val);

static int example_easy_static()
{
    u_hmap_t *hmap = NULL;
    char *vals[] = { "zero", "one", "two", "three" };

    u_dbg("example_easy_static()");

    dbg_err_if (u_hmap_easy_new(&hmap));

    dbg_err_if (u_hmap_easy_put(hmap, "0", vals[0]));
    dbg_err_if (u_hmap_easy_put(hmap, "1", vals[1]));
    dbg_err_if (u_hmap_easy_put(hmap, "2", vals[2]));
    dbg_err_if (u_hmap_easy_put(hmap, "3", vals[4]));

    /* value that doesn't exist */
    dbg_err_if (u_hmap_easy_get(hmap, "4") != NULL);

    /* test deletion */
    dbg_err_if (u_hmap_easy_del(hmap, "3"));
    dbg_err_if (u_hmap_easy_get(hmap, "3") != NULL);
    dbg_err_if (u_hmap_easy_put(hmap, "3", "THREE"));

    /* print out all values */
    dbg_err_if (strcmp(u_hmap_easy_get(hmap, "0"), vals[0]) != 0);
    dbg_err_if (strcmp(u_hmap_easy_get(hmap, "1"), vals[1]) != 0);
    dbg_err_if (strcmp(u_hmap_easy_get(hmap, "2"), vals[2]) != 0);
    dbg_err_if (strcmp(u_hmap_easy_get(hmap, "3"), "THREE") != 0);

    /* test overwrite - should fail */
    dbg_err_if (u_hmap_easy_put(hmap, "2", "TWO") == 0);
    dbg_err_if (strcmp(u_hmap_easy_get(hmap, "2"), "two") != 0);

    u_hmap_easy_free(hmap);

    return 0;
err:
    if (hmap)
        u_hmap_easy_free(hmap);

    return ~0;
}

struct mystruct_s {
    char *a;
    char *b;
};
typedef struct mystruct_s mystruct_t;

void mystruct_free (u_hmap_o_t *obj)
{
    mystruct_t *myval;

    if (obj == NULL || 
            obj->val == NULL)
        return;

    /* key is static so we only need to free value */
    myval = obj->val;

    u_free(myval->a);
    u_free(myval->b);
    u_free(myval);
};

mystruct_t *mystruct_create()
{
    mystruct_t *myval = (mystruct_t *) u_zalloc(sizeof(mystruct_t));
    myval->a = strdup("first string");
    myval->b = strdup("second string");
    return myval;
}

static int example_easy_dynamic()
{
    u_hmap_t *hmap = NULL;
    mystruct_t *mystruct;

    u_dbg("example_easy_dynamic()");

    dbg_err_if (u_hmap_easy_new(&hmap));

    /* setup custom free function */
    dbg_err_if (u_hmap_easy_set_freefunc(hmap, &mystruct_free));

    /* insert 3 objects */
    dbg_err_if (u_hmap_easy_put(hmap, "a", mystruct_create()));
    dbg_err_if (u_hmap_easy_put(hmap, "b", mystruct_create()));
    dbg_err_if (u_hmap_easy_put(hmap, "c", mystruct_create()));

    /* test overwrite - should fail */
    dbg_err_if (u_hmap_easy_put(hmap, "b", mystruct_create()) == 0);

    /* check a value */
    dbg_err_if ((mystruct = u_hmap_easy_get(hmap, "a")) == NULL);
    dbg_err_if (strcmp(mystruct->a, "first string") != 0);
    dbg_err_if (strcmp(mystruct->b, "second string") != 0);
    
    /* internal objects freed automatically using custom function */
    u_hmap_easy_free(hmap);

    return 0;
err:

    if (hmap)
        u_hmap_easy_free(hmap);

    return ~0;
}

static int example_static()
{
    u_hmap_t *hmap = NULL;
    u_hmap_o_t *obj = NULL;
    int fibonacci[] = { 0, 1, 1, 2, 3, 5, 8, 13, 21 };

    u_dbg("example_static()");

    /* initialise hmap with no options - user owns data by default */
    dbg_err_if (u_hmap_new(NULL, &hmap));

    /* insert some sample elements */
    dbg_err_if (u_hmap_put(hmap, u_hmap_o_new("first", &fibonacci[0]), NULL));
    dbg_err_if (u_hmap_put(hmap, u_hmap_o_new("fifth", &fibonacci[4]), NULL)); 
    dbg_err_if (u_hmap_put(hmap, u_hmap_o_new("last", 
                &fibonacci[(sizeof(fibonacci)/sizeof(int))-1]), NULL));

    /* retrieve and print values to dbgsole */
    dbg_err_if (u_hmap_get(hmap, "last", &obj)); 
    u_dbg("hmap['%s'] = %d", (char *) obj->key, *((int *) obj->val));
    dbg_err_if (u_hmap_get(hmap, "fifth", &obj)); 
    u_dbg("hmap['%s'] = %d", (char *) obj->key, *((int *) obj->val)); 
    dbg_err_if (u_hmap_get(hmap, "first", &obj)); 
    u_dbg("hmap['%s'] = %d", (char *) obj->key, *((int *) obj->val));

    /* remove an element and replace it */
    dbg_err_if (u_hmap_del(hmap, "fifth", &obj)); 
    u_hmap_o_free(obj);
    
    /* check that it has been deleted */
    dbg_err_if (u_hmap_get(hmap, "fifth", &obj) == 0); 

    /* delete the other two elements */
    dbg_err_if (u_hmap_del(hmap, "last", &obj)); 
    u_hmap_o_free(obj);
    dbg_err_if (u_hmap_del(hmap, "first", &obj)); 
    u_hmap_o_free(obj);

    /* free hmap and options */
    u_hmap_free(hmap);
    
    return U_TEST_EXIT_SUCCESS;
err:
    U_FREEF(hmap, u_hmap_free);

    return U_TEST_EXIT_FAILURE;
}

static int example_dynamic_own_hmap (void)
{
    u_hmap_opts_t *opts = NULL;
    u_hmap_t *hmap = NULL;
    u_hmap_o_t *obj = NULL;

    u_dbg("example_dynamic_own_hmap()");

    /* initialise options and hmap */
    dbg_err_if (u_hmap_opts_new(&opts));
    /* hmap owns both keys and data */
    opts->options |= U_HMAP_OPTS_OWNSDATA;
    dbg_err_if (u_hmap_new(opts, &hmap));

    /* insert some sample elements */
    dbg_err_if (u_hmap_put(hmap, u_hmap_o_new((void *) u_strdup("english"), 
                    (void *) u_strdup("Hello world!")), NULL));
    dbg_err_if (u_hmap_put(hmap, u_hmap_o_new((void *) u_strdup("italian"), 
                    (void *) u_strdup("Ciao mondo!")), NULL));
    dbg_err_if (u_hmap_put(hmap, u_hmap_o_new((void *) u_strdup("german"), 
                    (void *) u_strdup("Hallo Welt!")), NULL));

    /* retrieve and print values to console */
    dbg_err_if (u_hmap_get(hmap, "italian", &obj)); 
    u_dbg("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);
    dbg_err_if (u_hmap_get(hmap, "german", &obj)); 
    u_dbg("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);
    dbg_err_if (u_hmap_get(hmap, "english", &obj)); 
    u_dbg("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);

    /* remove an element and replace it */
    dbg_err_if (u_hmap_del(hmap, "german", NULL)); 

    /* check that it has been deleted */
    dbg_err_if (u_hmap_get(hmap, "german", &obj) == 0); 

    /* replace with a new element and print it */
    dbg_err_if (u_hmap_put(hmap, u_hmap_o_new((void *) u_strdup("german"), 
                    (void *) u_strdup("Auf Wiedersehen!")), NULL));
    dbg_err_if (u_hmap_get(hmap, "german", &obj)); 
    u_dbg("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);

    /* free hmap (options and elements are freed automatically) */
    u_hmap_dbg(hmap);
    u_hmap_opts_free(opts);
    u_hmap_free(hmap);

    return U_TEST_EXIT_SUCCESS;

err:
    U_FREEF(opts, u_hmap_opts_free);
    U_FREEF(hmap, u_hmap_free);

    return U_TEST_EXIT_FAILURE;
}

static int example_dynamic_own_user (void)
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
    
    u_dbg("example_dynamic_own_user()");

    /* hmap owns both keys and data */
    dbg_err_if (u_hmap_new(NULL, &hmap));

    /* insert some sample elements */
    dbg_err_if (u_hmap_put(hmap, u_hmap_o_new((void *) u_strdup("english"), 
                    (void *) u_strdup("Hello world!")), &obj));
    dbg_err_if (u_hmap_put(hmap, u_hmap_o_new((void *) u_strdup("italian"), 
                    (void *) u_strdup("Ciao mondo!")), &obj));
    dbg_err_if (u_hmap_put(hmap, u_hmap_o_new((void *) u_strdup("german"), 
                    (void *) u_strdup("Hallo Welt!")), &obj));

    /* retrieve and print values to console */
    dbg_err_if (u_hmap_get(hmap, "italian", &obj)); 
    u_dbg("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);
    dbg_err_if (u_hmap_get(hmap, "german", &obj)); 
    u_dbg("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);
    dbg_err_if (u_hmap_get(hmap, "english", &obj)); 
    u_dbg("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);

    /* remove an element and replace it */
    dbg_err_if (u_hmap_del(hmap, "german", &obj)); 
    OBJ_FREE(obj);

    /* check that it has been deleted */
    dbg_err_if (u_hmap_get(hmap, "german", &obj) == 0); 

    /* replace with a new element and print it */
    dbg_err_if (u_hmap_put(hmap, u_hmap_o_new((void *) u_strdup("german"), 
                    (void *) u_strdup("Auf Wiedersehen!")), NULL));
    dbg_err_if (u_hmap_put(hmap, u_hmap_o_new((void *) u_strdup("german"), 
                    (void *) u_strdup("Auf Wiedersehen2!")), &obj));
    OBJ_FREE(obj);
    dbg_err_if (u_hmap_get(hmap, "german", &obj)); 
    u_dbg("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);

    u_hmap_del(hmap, "italian", &obj);
    OBJ_FREE(obj);
    u_hmap_del(hmap, "german", &obj);
    OBJ_FREE(obj);
    u_hmap_del(hmap, "english", &obj);
    OBJ_FREE(obj);

    /* free hmap (options and elements are freed automatically) */
    u_hmap_free(hmap);

    return U_TEST_EXIT_SUCCESS;

err:
    U_FREEF(hmap, u_hmap_free);

    return U_TEST_EXIT_FAILURE;
}

static int example_no_overwrite (void)
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

    u_dbg("example_no_overwrite()");

    /* initialise options and hmap */
    dbg_err_if (u_hmap_opts_new(&opts));
    /* hmap owns both keys and data */
    opts->options |= U_HMAP_OPTS_NO_OVERWRITE;
    dbg_err_if (u_hmap_new(opts, &hmap));

    /* insert some sample elements with the same key */
    MAP_INSERT(hmap, "A", "A1", obj);
    MAP_INSERT(hmap, "A", "A2", obj);
    MAP_INSERT(hmap, "A", "A3", obj);

    /* retrieve and print values to console */
    dbg_err_if (u_hmap_get(hmap, "A", &obj)); 
    u_dbg("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);
    dbg_err_if (u_hmap_del(hmap, "A", &obj)); 
    u_hmap_o_free(obj); 

    /* free hmap and opts */
    u_hmap_opts_free(opts);
    u_hmap_free(hmap);

    return U_TEST_EXIT_SUCCESS;
err:
    U_FREEF(opts, u_hmap_opts_free);
    U_FREEF(hmap, u_hmap_free);

    return U_TEST_EXIT_FAILURE;
#undef MAP_INSERT
}

static size_t __sample_hash(void *key, size_t size)
{
    return (*((int *) key) % size);
}

static int __sample_comp(void *key1, void *key2)
{
    int k1 = *((int *) key1),
        k2 = *((int *) key2);
    
    return k1 < k2 ? -1 : ((k1 > k2)? 1 : 0);
}

static u_string_t *__sample_str(u_hmap_o_t *obj)
{
    enum { MAX_OBJ_STR = 256 };
    char buf[MAX_OBJ_STR];
    u_string_t *s = NULL;

    int key = *((int *) obj->key);
    char *val = (char *) obj->val;

    dbg_err_if (u_snprintf(buf, MAX_OBJ_STR, "[%d:%s]", key, val));
    dbg_err_if (u_string_create(buf, strlen(buf)+1, &s));

    return s;

err:
    return NULL;
}

/* Allocate (key, value) pair dynamically */
static u_hmap_o_t *__sample_obj(int key, const char *val)
{
    u_hmap_o_t *new = NULL;

    int *k = NULL;
    char *v = NULL;
    
    k = (int *) malloc(sizeof(int));
    dbg_err_if (k == NULL);
    *k = key;

    v = u_strdup(val);
    dbg_err_if (v == NULL);
    
    new = u_hmap_o_new(k, v);
    dbg_err_if (new == NULL);
    
    return new;

err:
    u_free(k);
    u_free(v);
    
    return NULL;
}

static int example_types_custom (void)
{
#define MAP_INSERT(hmap, k, v) \
    dbg_err_if ((obj = __sample_obj(k, v)) == NULL); \
    dbg_err_if (u_hmap_put(hmap, obj, NULL));

    u_hmap_opts_t *opts = NULL;
    u_hmap_t *hmap = NULL;
    u_hmap_o_t *obj = NULL; 
    
    u_dbg("example_types_custom()"); 

    dbg_err_if (u_hmap_opts_new(&opts));
    opts->options |= U_HMAP_OPTS_OWNSDATA | U_HMAP_OPTS_HASH_STRONG;
    opts->size = 3;
    opts->f_hash = &__sample_hash;
    opts->f_comp = &__sample_comp;
    opts->f_str = &__sample_str;

    dbg_err_if (u_hmap_new(opts, &hmap));

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
    dbg_err_if (u_hmap_get(hmap, &x, &obj)); 
    u_dbg("hmap['%d'] = %s", *((int *) obj->key), (char *) obj->val);
    x++;
    dbg_err_if (u_hmap_get(hmap, &x, &obj)); 
    u_dbg("hmap['%d'] = %s", *((int *) obj->key), (char *) obj->val);
    x++;
    dbg_err_if (u_hmap_get(hmap, &x, &obj)); 
    u_dbg("hmap['%d'] = %s", *((int *) obj->key), (char *) obj->val);
    
    u_hmap_dbg(hmap);
    u_hmap_opts_free(opts);
    u_hmap_free(hmap);

    return U_TEST_EXIT_SUCCESS;
err:
    U_FREEF(opts, u_hmap_opts_free);
    U_FREEF(hmap, u_hmap_free);

    return U_TEST_EXIT_FAILURE;
#undef MAP_INSERT
}

static int test_resize (void)
{
    enum { NUM_ELEMS = 100000, MAX_STR = 256 };
    u_hmap_opts_t *opts = NULL;
    u_hmap_t *hmap = NULL;
    u_hmap_o_t *obj = NULL;
    int i = 0;
    char key[MAX_STR], 
         val[MAX_STR];

    u_dbg("test_resize()");

    /* initialise hmap with no options - user owns data by default */
    dbg_err_if (u_hmap_opts_new(&opts));
    opts->size = 3;
    dbg_err_if (u_hmap_new(opts, &hmap));

    /* insert some sample elements */
    for (i = 0; i < NUM_ELEMS; ++i) {
        u_snprintf(key, MAX_STR, "key%d", i);
        u_snprintf(val, MAX_STR, "val%d", i);
        dbg_err_if (u_hmap_put(hmap, u_hmap_o_new(u_strdup(key), 
                        u_strdup(val)), NULL));
    }

    for (i = 0; i < NUM_ELEMS; ++i) {
        u_snprintf(key, MAX_STR, "key%d", i);
        dbg_err_if (u_hmap_del(hmap, key, &obj)); 
        u_hmap_o_free(obj->key);
        u_hmap_o_free(obj->val);
        u_hmap_o_free(obj);
    }

    /* free hmap and options */
    u_hmap_opts_free(opts);
    u_hmap_free(hmap);
    
    return U_TEST_EXIT_SUCCESS;

err:
    U_FREEF(opts, u_hmap_opts_free);
    U_FREEF(hmap, u_hmap_free);

    return U_TEST_EXIT_FAILURE;
}

static int test_linear (void)
{
    enum { NUM_ELEMS = 100000, MAX_STR = 256 };
    u_hmap_opts_t *opts = NULL;
    u_hmap_t *hmap = NULL;
    u_hmap_o_t *obj = NULL;
    int i = 0;
    char key[MAX_STR], 
         val[MAX_STR];

    u_dbg("test_linear()");

    /* initialise hmap with no options - user owns data by default */
    dbg_err_if (u_hmap_opts_new(&opts));
    opts->size = 1000;
    opts->type = U_HMAP_TYPE_LINEAR;
    dbg_err_if (u_hmap_new(opts, &hmap));

    /* insert some sample elements */
    for (i = 0; i < NUM_ELEMS; ++i) {
        u_snprintf(key, MAX_STR, "key%d", i);
        u_snprintf(val, MAX_STR, "val%d", i);
        dbg_err_if (u_hmap_put(hmap, u_hmap_o_new(u_strdup(key), 
                        u_strdup(val)), NULL));
    }

    for (i = 0; i < NUM_ELEMS; ++i) {
        u_snprintf(key, MAX_STR, "key%d", i);
        dbg_err_if (u_hmap_del(hmap, key, &obj)); 
        u_hmap_o_free(obj->key);
        u_hmap_o_free(obj->val);
        u_hmap_o_free(obj);
    }

    /* free hmap and options */
    u_hmap_opts_free(opts);
    u_hmap_free(hmap);
    
    return U_TEST_EXIT_SUCCESS;

err:
    U_FREEF(opts, u_hmap_opts_free);
    U_FREEF(hmap, u_hmap_free);

    return U_TEST_EXIT_FAILURE;
}

U_TEST_SUITE(hmap)
{
    /* examples */
    U_TEST_CASE_ADD( example_easy_static );
    U_TEST_CASE_ADD( example_easy_dynamic );
    U_TEST_CASE_ADD( example_static );
    U_TEST_CASE_ADD( example_dynamic_own_hmap );
    U_TEST_CASE_ADD( example_dynamic_own_user );
    U_TEST_CASE_ADD( example_no_overwrite );
    U_TEST_CASE_ADD( example_types_custom );

    /* tests */
    U_TEST_CASE_ADD( test_resize );
    U_TEST_CASE_ADD( test_linear );

    return 0;
}
