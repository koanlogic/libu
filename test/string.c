#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <u/libu.h>

int test_suite_string_register (u_test_t *t);

static int test_u_str (u_test_case_t *tc);

static int test_u_str (u_test_case_t *tc)
{
    u_string_t *s = NULL;

    u_test_err_if (u_string_create("0", 1, &s));

    u_test_err_if (strcmp(u_string_c(s), "0"));

    u_test_err_if (u_string_sprintf(s, "%s", "1"));
    u_test_err_if (strcmp(u_string_c(s), "1"));

    u_test_err_if (u_string_aprintf(s, "%s", "23"));
    u_test_err_if (strcmp(u_string_c(s), "123"));

    u_test_err_if (u_string_cat(s, "45"));
    u_test_err_if (strcmp(u_string_c(s), "12345"));

    u_test_err_if (u_string_ncat(s, "6777", 2));
    u_test_err_if (strcmp(u_string_c(s), "1234567"));

    u_test_err_if (u_string_sprintf(s, "%s", "reset"));
    u_test_err_if (strcmp(u_string_c(s), "reset"));

    u_string_free(s);

    return U_TEST_SUCCESS;
err:
    return U_TEST_FAILURE;
}

int test_suite_string_register (u_test_t *t)
{
    u_test_suite_t *ts = NULL;

    con_err_if (u_test_suite_new("Strings", &ts));

    con_err_if (u_test_case_register("Various functions", test_u_str, ts));

    return u_test_suite_add(ts, t);
err:
    u_test_suite_free(ts);
    return ~0;
}
