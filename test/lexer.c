#include <u/libu.h>

int test_suite_lexer_register (u_test_t *t);

static int test_0 (u_test_case_t *tc);

static int test_0 (u_test_case_t *tc)
{
    u_test_err_ifm (0, "0");

    return U_TEST_SUCCESS;
err:
    return U_TEST_FAILURE;
}

int test_suite_lexer_register (u_test_t *t)
{
    u_test_suite_t *ts = NULL;

    con_err_if (u_test_suite_new("Lexer", &ts));

    con_err_if (u_test_case_register("0", test_0, ts));

    return u_test_suite_add(ts, t);
err:
    u_test_suite_free(ts);
    return ~0;
}
