#include <u/libu.h>

int test_suite_bst_register (u_test_t *t);

static int test_u_bst (u_test_case_t *tc);

static int test_u_bst (u_test_case_t *tc)
{
    /* TODO */
    return U_TEST_SUCCESS;
}

int test_suite_pwd_register (u_test_t *t)
{
    u_test_suite_t *ts = NULL;

    con_err_if (u_test_suite_new("Binary Search Tree", &ts));

    con_err_if (u_test_case_register("TODO", test_u_bst, ts));

    return u_test_suite_add(ts, t);
err:
    u_test_suite_free(ts);
    return ~0;
}
