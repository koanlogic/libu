#include <u/libu.h>

int test_suite_json_register (u_test_t *t);

static int test_codec (u_test_case_t *tc);
static int test_build_simple_object (u_test_case_t *tc);
static int test_build_nested_object (u_test_case_t *tc);
static int test_build_simple_array (u_test_case_t *tc);
static int test_iterators (u_test_case_t *tc);
static int test_max_nesting (u_test_case_t *tc);

static int test_codec (u_test_case_t *tc)
{
    size_t i;
    char *s = NULL;
    u_json_t *jo = NULL;

    const char *tv[] = {
        /* Empty object. */
        "{  }",  
        /* Empty array. */
        "[  ]",  
        /* Nesting. */
        "[ {  }, {  }, [ [  ], {  } ] ]",   
        /* ASCII String. */ 
        "{ \"ascii\": \"This is an ASCII string.\" }",
        /* UNICODE String. */
        "{ \"unicode\": \"This is a \\uDEAD\\uBEEF.\" }",
        /* Integer. */ 
        "{ \"int\": 12439084123 }",
        /* Exp. */ 
        "{ \"exp\": -12439084123E+1423 }",
        /* Frac. */ 
        "{ \"frac\": 12439084123.999e-1423 }",
        /* Boolean. */
        "[ true, false ]",
        /* Null. */
        "{ \"NullMatrix\": [ [ null, null ], [ null, null ] ] }",
        NULL
    };

    for (i = 0; tv[i] != NULL; i++)
    {
        u_test_err_if (u_json_decode(tv[i], &jo)); 
        u_test_err_if (u_json_encode(jo, &s));
        u_test_err_ifm (strcmp(s, tv[i]), "%s and %s differ !", tv[i], s);

        u_free(s), s = NULL;
        u_json_free(jo), jo = NULL;
    }

    return U_TEST_SUCCESS;
err:
    if (s)
        u_free(s);
    if (jo)
        u_json_free(jo);

    return U_TEST_FAILURE;
}

static int test_build_simple_array (u_test_case_t *tc)
{
    long l;
    char *s = NULL;
    u_json_t *root = NULL, *tmp = NULL;
    const char *ex = "[ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 ]";

    /* [ ... ] */
    u_test_err_if (u_json_new_array(NULL, &root));

    for (l = 0; l < 10 ; l++)
    {
        /* "$i," */ 
        u_test_err_if (u_json_new_int(NULL, l, &tmp));
        u_test_err_if (u_json_add(root, tmp));
        tmp = NULL;
    }

    u_test_err_if (u_json_encode(root, &s));

    u_test_err_ifm (strcmp(ex, s), "expecting \'%s\', got \'%s\'", ex, s);
    
    u_json_free(root);
    u_free(s);

    return U_TEST_SUCCESS;
err:
    if (root)
        u_json_free(root);
    if (tmp)
        u_json_free(tmp);
    if (s)
        u_free(s);

    return U_TEST_FAILURE;
}

static int test_build_simple_object (u_test_case_t *tc)
{
    char *s = NULL;
    u_json_t *root = NULL, *tmp = NULL;
    const char *ex = 
        "{ \"num\": 999, \"string\": \".\", \"null\": null, \"bool\": true }";

    /* { ... } */
    u_test_err_if (u_json_new_object(NULL, &root));

    /* "num": "999" */
    u_test_err_if (u_json_new_int("num", 999, &tmp));
    u_test_err_if (u_json_add(root, tmp));
    tmp = NULL;

    /* "string": "." */
    u_test_err_if (u_json_new_string("string", ".", &tmp));
    u_test_err_if (u_json_add(root, tmp));
    tmp = NULL;

    /* "null": null */
    u_test_err_if (u_json_new_null("null", &tmp));
    u_test_err_if (u_json_add(root, tmp));
    tmp = NULL;

    /* "bool": true */
    u_test_err_if (u_json_new_bool("bool", 1, &tmp));
    u_test_err_if (u_json_add(root, tmp));
    tmp = NULL;

    u_test_err_if (u_json_encode(root, &s));

    u_test_err_ifm (strcmp(ex, s), "expecting \'%s\', got \'%s\'", ex, s);
 
    u_json_free(root);
    u_free(s);

    return U_TEST_SUCCESS;
err:
    if (root)
        u_json_free(root);
    if (tmp)
        u_json_free(tmp);
    if (s)
        u_free(s);

    return U_TEST_FAILURE;
}

static int test_build_nested_object (u_test_case_t *tc)
{
    int i;
    char *s = NULL;
    u_json_t *array = NULL, *root = NULL, *tmp = NULL;
    const char *ex = "{ \"array\": [ null, null, null ] }";

    /* Nested array of null's. */
    u_test_err_if (u_json_new_array("array", &array));

    for (i= 0; i < 3 ; i++)
    {
        u_test_err_if (u_json_new_null(NULL, &tmp));
        u_test_err_if (u_json_add(array, tmp));
        tmp = NULL;
    }

    /* TODO add nested simple object. */

    /* Top level container. */
    u_test_err_if (u_json_new_object(NULL, &root));
    u_test_err_if (u_json_add(root, array));
    array = NULL;

    u_test_err_if (u_json_encode(root, &s));
    
    u_test_err_ifm (strcmp(ex, s), "expecting \'%s\', got \'%s\'", ex, s);

    u_json_free(root);
    u_free(s);

    return U_TEST_SUCCESS;
err:
    if (root)
        u_json_free(root);
    if (array)
        u_json_free(array);
    if (tmp)
        u_json_free(tmp);
    if (s)
        u_free(s);

    return U_TEST_FAILURE;
}

static int test_iterators (u_test_case_t *tc)
{
    long e, i;
    u_json_it_t jit;
    u_json_t *jo = NULL, *cur;
    const char *s = "[ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ]";

    u_test_err_if (u_json_decode(s, &jo));

    /* Init array iterator from first element and go forward. */
    u_test_err_if (u_json_it(u_json_child_first(jo), &jit));
    for (i = 1; (cur = u_json_it_next(&jit)) != NULL; i++)
    {
        u_test_err_if (u_json_get_int(cur, &e));
        u_test_err_ifm (e != i, "expecting \'%d\', got \'%d\'", e, i);
    }

    /* Init array iterator from last element and go backwards. */
    u_test_err_if (u_json_it(u_json_child_last(jo), &jit));
    for (i = 10; (cur = u_json_it_prev(&jit)) != NULL; i--)
    {
        u_test_err_if (u_json_get_int(cur, &e));
        u_test_err_ifm (e != i, "expecting \'%d\', got \'%d\'", e, i);
    }

    u_json_free(jo), jo = NULL;

    return U_TEST_SUCCESS;
err:
    return U_TEST_FAILURE;
}

static int test_max_nesting (u_test_case_t *tc)
{
    u_json_t *jo = NULL;
    char ns[(U_JSON_MAX_DEPTH * 2) + 2 + 1];

    /*
     * [[[[[[[[[[[[[[[[[[[...[]...]]]]]]]]]]]]]]]]]]] 
     *  `--U_JSON_MAX_DEPTH--'                        
     */
    memset(ns, '[', U_JSON_MAX_DEPTH + 1);
    memset(ns + U_JSON_MAX_DEPTH + 1, ']', U_JSON_MAX_DEPTH + 1);
    ns[(U_JSON_MAX_DEPTH * 2) + 2] = '\0';

    /* Try to parse it (should fail). */
    u_test_err_ifm (u_json_decode(ns, &jo) == 0, 
            "expecting parser rejection because of excessive nesting");

    u_json_free(jo);

    return U_TEST_SUCCESS;
err:
    if (jo)
        u_json_free(jo);

    return U_TEST_FAILURE;
}

int test_suite_json_register (u_test_t *t)
{
    u_test_suite_t *ts = NULL;

    con_err_if (u_test_suite_new("JSON", &ts));

    con_err_if (u_test_case_register("Encode-Decode", test_codec, ts));
    con_err_if (u_test_case_register("Builder (simple object)", 
                test_build_simple_object, ts));
    con_err_if (u_test_case_register("Builder (simple array)", 
                test_build_simple_array, ts));
    con_err_if (u_test_case_register("Builder (nested object)", 
                test_build_nested_object, ts));
    con_err_if (u_test_case_register("Iterators", test_iterators, ts));
    con_err_if (u_test_case_register("Nesting", test_max_nesting, ts));

    /* JSON depends on the lexer and hmap modules. */
    con_err_if (u_test_suite_dep_register("Lexer", ts));
    con_err_if (u_test_suite_dep_register("Hash Map", ts));

    return u_test_suite_add(ts, t);
err:
    u_test_suite_free(ts);
    return ~0;
}
