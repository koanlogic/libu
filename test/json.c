#include <u/libu.h>

int test_suite_json_register (u_test_t *t);

static int test_codec (u_test_case_t *tc);
static int test_build (u_test_case_t *tc);

static int test_codec (u_test_case_t *tc)
{
    size_t i;
    char *s = NULL;
    u_json_obj_t *jo = NULL;

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
        u_json_obj_free(jo), jo = NULL;
    }

    return U_TEST_SUCCESS;
err:
    if (s)
        u_free(s);
    if (jo)
        u_json_obj_free(jo);

    return U_TEST_FAILURE;
}

static int test_build (u_test_case_t *tc)
{
    u_json_obj_t *root = NULL, *tmp = NULL;

    u_test_err_if (u_json_obj_new(&root));
     


    u_json_obj_free(root);

    return U_TEST_SUCCESS;
 err:
    if (root)
        u_json_obj_free(root);

    return U_TEST_FAILURE;
}


int test_suite_json_register (u_test_t *t)
{
    u_test_suite_t *ts = NULL;

    con_err_if (u_test_suite_new("JSON", &ts));

    con_err_if (u_test_case_register("Encode-Decode", test_codec, ts));
    con_err_if (u_test_case_register("Builder", test_build, ts));

    /* JSON depends on the lexer and hmap modules. */
    con_err_if (u_test_suite_dep_register("Lexer", ts));
    con_err_if (u_test_suite_dep_register("Hash Map", ts));

    return u_test_suite_add(ts, t);
err:
    u_test_suite_free(ts);
    return ~0;
}
