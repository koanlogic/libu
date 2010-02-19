#include <u/libu.h>
#include "test.h"

int facility = LOG_LOCAL0;

int TC_1_1 (test_case_t *tc);
int TC_1_2 (test_case_t *tc);
int TC_2_1 (test_case_t *tc);
int TC_2_2 (test_case_t *tc);

int test_suite_TS1_register (test_t *t);
int test_suite_TS2_register (test_t *t);

int main (int argc, char *argv[])
{
    test_t *t = NULL;

    con_err_if (test_new("my test", &t));
    con_err_if (test_suite_TS1_register(t));
    con_err_if (test_suite_TS2_register(t));
    con_err_if (test_run(argc, argv, t));

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
    con_err_if (test_case_register("TC 1.1", TC_1_1, ts));
    con_err_if (test_case_register("TC 1.2", TC_1_2, ts));
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
    con_err_if (test_case_register("TC 2.1", TC_2_1, ts));
    con_err_if (test_case_register("TC 2.2", TC_2_2, ts));
    con_err_if (test_suite_dep_register("TS 1", ts));

    return test_suite_add(ts, t);
err:
    test_suite_free(ts);
    return ~0;
}



/*
 * fake test cases
 */
int TC_1_1 (test_case_t *tc)
{
    u_con("hello TC_1_1");
    return TEST_SUCCESS;
}

int TC_1_2 (test_case_t *tc)
{
    u_con("hello TC_1_2 (sleeping)");
    sleep(5);
    return TEST_FAILURE;
}

int TC_2_1 (test_case_t *tc)
{
    u_con("hello TC_2_1");
    sleep(2);
    return TEST_SUCCESS;
}

int TC_2_2 (test_case_t *tc)
{
    char r[1];
    u_con("hello TC_2_2");
    abort();
    return TEST_SUCCESS;
}
