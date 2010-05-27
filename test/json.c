#include <u/libu.h>

int test_suite_json_register (u_test_t *t);

static int test_json (u_test_case_t *tc);

static int test_json (u_test_case_t *tc)
{
    u_test_case_printf(tc, "TODO");

    return U_TEST_SUCCESS;
}

int test_suite_json_register (u_test_t *t)
{
    u_test_suite_t *ts = NULL;

    con_err_if (u_test_suite_new("JSON", &ts));

    con_err_if (u_test_case_register("TODO", test_json, ts));

    return u_test_suite_add(ts, t);
err:
    u_test_suite_free(ts);
    return ~0;
}
