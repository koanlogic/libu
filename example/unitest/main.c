#include <u/libu.h>
#include "test.h"

int facility = LOG_LOCAL0;

int test_suite_TS1_register (test_t *t);
int test_suite_TS2_register (test_t *t);

int main (void)
{
    test_t *t = NULL;

    con_err_if (test_new("my test", &t));
    con_err_if (test_suite_TS1_register(t));
    con_err_if (test_suite_TS2_register(t));
    con_err_if (test_run(t));

    test_free(t);

    return EXIT_SUCCESS;
err:
    test_free(t);
    return EXIT_FAILURE;
}

/* should go in a separate file */

int test_suite_TS1_register (test_t *t)
{
    test_suite_t *ts = NULL;

    /* 
     * - test suite "TS 1":
     *      - test case "TC 1.1" which depends on "TC 1.2"
     *      - test case "TC 1.2"
     */
    con_err_if (test_suite_new("TS 1", &ts));
    con_err_if (test_case_register("TC 1.1", NULL, ts));
    con_err_if (test_case_register("TC 1.2", NULL, ts));
    con_err_if (test_case_depends_on("TC 1.1", "TC 1.2", ts));

    return test_suite_add(ts, t);
err:
    test_suite_free(ts);
    return ~0;
}

int test_suite_TS2_register (test_t *t)
{
    test_suite_t *ts = NULL;

    /*
     *  - test suite "TS 2" which depends on "TS 1"
     *      - test case "TC 2.1"
     *      - test case "TC 2.2"
     */
    con_err_if (test_suite_new("TS 2", &ts));
    con_err_if (test_case_register("TC 2.1", NULL, ts));
    con_err_if (test_case_register("TC 2.2", NULL, ts));
    con_err_if (test_suite_dep_register("TS 1", ts));   

    return test_suite_add(ts, t);
err:
    test_suite_free(ts);
    return ~0;
}
