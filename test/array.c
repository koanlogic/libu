#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <u/libu.h>

int test_suite_array_register (u_test_t *t);

static int test_resize (u_test_case_t *tc);
static int test_short (u_test_case_t *tc);
static int test_ptr (u_test_case_t *tc);
static int test_u_short (u_test_case_t *tc);
static int test_char (u_test_case_t *tc);
static int test_u_char (u_test_case_t *tc);

static int test_resize (u_test_case_t *tc)
{
    u_array_t *da = NULL;
    size_t idx;
    short s, s0;

    u_test_err_if (u_array_create(U_ARRAY_TYPE_SHORT, 100, &da));

    for (s = SHRT_MIN, idx = 0; s < SHRT_MAX; s++, idx++)
    {
        u_test_err_if (u_array_set_short(da, idx, s, NULL));
        u_test_err_if (u_array_get_short(da, idx, &s0));
        u_test_err_ifm (s != s0, "s = %d, s0 = %d, idx = %zu", s, s0, idx);
    }

    u_array_free(da);

    return U_TEST_SUCCESS;
err:
    u_array_free(da);

    return U_TEST_FAILURE;
}

static int test_short (u_test_case_t *tc)
{
    u_array_t *da = NULL;
    size_t idx;
    short s, s0;

    u_test_err_if (u_array_create(U_ARRAY_TYPE_SHORT, SHRT_MAX * 2 + 1, &da));

    for (s = SHRT_MIN, idx = 0; s < SHRT_MAX; s++, idx++)
    {
        u_test_err_if (u_array_set_short(da, idx, s, NULL));
        u_test_err_if (u_array_get_short(da, idx, &s0));
        u_test_err_ifm (s != s0, "s = %d, s0 = %d, idx = %zu", s, s0, idx);
    }

    u_array_free(da);

    return U_TEST_SUCCESS;
err:
    u_array_free(da);

    return U_TEST_FAILURE;
}

static int test_ptr (u_test_case_t *tc)
{
    int rc = 0;
    u_array_t *da = NULL;
    size_t idx;
    struct S { int i; char c; } s, *s0;

    u_test_err_if (u_array_create(U_ARRAY_TYPE_PTR, 10, &da));

    for (idx = 0; idx < 100; idx++)
    {
        s.i = (int) idx;
        s.c = (char) idx;

        /* explicitly ignore "old values" */
        (void) u_array_set_ptr(da, idx, &s, &rc);
        u_test_err_ifm (rc == -1, "setting %p at idx %zu failed", &s, idx);

        s0 = u_array_get_ptr(da, idx, &rc);
        u_test_err_ifm (rc == -1, "getting from idx %zu failed", idx);

        u_test_err_ifm (s.i != s0->i, "%d != %d at index %zu", s.i, s0->i, idx);
        u_test_err_ifm (s.c != s0->c, "%c != %c at index %zu", s.c, s0->c, idx);
    }

    u_array_free(da);

    return U_TEST_SUCCESS;
err:
    u_array_free(da);

    return U_TEST_FAILURE;
}

static int test_u_short (u_test_case_t *tc)
{
    u_array_t *da = NULL;
    size_t idx;
    unsigned short s, s0;

    u_test_err_if (u_array_create(U_ARRAY_TYPE_U_SHORT, USHRT_MAX + 1, &da));

    for (s = 0, idx = 0; s < USHRT_MAX; s++, idx++)
    {
        u_test_err_if (u_array_set_u_short(da, idx, s, NULL));
        u_test_err_if (u_array_get_u_short(da, idx, &s0));
        u_test_err_ifm (s != s0, "s = %d, s0 = %d, idx = %zu", s, s0, idx);
    }

    u_array_free(da);

    return U_TEST_SUCCESS;
err:
    u_array_free(da);

    return U_TEST_FAILURE;
}

static int test_char (u_test_case_t *tc)
{
    u_array_t *da = NULL;
    size_t idx;
    char s, s0;

    u_test_err_if (u_array_create(U_ARRAY_TYPE_CHAR, CHAR_MAX * 2 + 1, &da));

    for (s = CHAR_MIN, idx = 0; s < CHAR_MAX; s++, idx++)
    {
        u_test_err_if (u_array_set_char(da, idx, s, NULL));
        u_test_err_if (u_array_get_char(da, idx, &s0));
        u_test_err_ifm (s != s0, "s = %d, s0 = %d, idx = %zu", s, s0, idx);
    }

    u_array_free(da);

    return U_TEST_SUCCESS;
err:
    u_array_free(da);

    return U_TEST_FAILURE;
}

static int test_u_char (u_test_case_t *tc)
{
    size_t idx;
    u_array_t *da = NULL;
    unsigned char s, s0;

    u_test_err_if (u_array_create(U_ARRAY_TYPE_U_CHAR, UCHAR_MAX + 1, &da));

    for (s = 0, idx = 0; s < UCHAR_MAX; s++, idx++)
    {
        u_test_err_if (u_array_set_u_char(da, idx, s, NULL));
        u_test_err_if (u_array_get_u_char(da, idx, &s0));
        u_test_err_ifm (s != s0, "s = %d, s0 = %d, idx = %zu", s, s0, idx);
    }

    u_array_free(da);

    return U_TEST_SUCCESS;
err:
    u_array_free(da);

    return U_TEST_FAILURE;
}

int test_suite_array_register (u_test_t *t)
{
    u_test_suite_t *ts = NULL;

    con_err_if (u_test_suite_new("Dynamic Arrays", &ts));

    con_err_if (u_test_case_register("Pointer elements", test_ptr, ts));
    con_err_if (u_test_case_register("Short elements", test_short, ts));
    con_err_if (u_test_case_register("Unsigned short elements", 
                test_u_short, ts));
    con_err_if (u_test_case_register("Char elements", test_char, ts));
    con_err_if (u_test_case_register("Unsigned char elements", 
                test_u_char, ts));
    con_err_if (u_test_case_register("Force array resize", test_resize, ts));

    return u_test_suite_add(ts, t);
err:
    u_test_suite_free(ts);
    return ~0;
}
