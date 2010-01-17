#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <syslog.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <u/libu.h>

U_TEST_SUITE(array);

static int test_resize (void);
static int test_short (void);
static int test_ptr (void);
static int test_u_short (void);
static int test_char (void);
static int test_u_char (void);

static int test_resize (void)
{
    u_array_t *da = NULL;
    size_t idx;
    short s, s0;

    con_err_if (u_array_create(U_ARRAY_TYPE_SHORT, 100, &da));

    for (s = SHRT_MIN, idx = 0; s < SHRT_MAX; s++, idx++)
    {
        con_err_if (u_array_set_short(da, idx, s, NULL));
        con_err_if (u_array_get_short(da, idx, &s0));
        con_err_ifm (s != s0, "s = %d, s0 = %d, idx = %zu", s, s0, idx);
    }

    u_array_free(da);

    return 0;
err:
    u_array_free(da);

    return -1;
}

static int test_short (void)
{
    u_array_t *da = NULL;
    size_t idx;
    short s, s0;

    con_err_if (u_array_create(U_ARRAY_TYPE_SHORT, SHRT_MAX * 2 + 1, &da));

    for (s = SHRT_MIN, idx = 0; s < SHRT_MAX; s++, idx++)
    {
        con_err_if (u_array_set_short(da, idx, s, NULL));
        con_err_if (u_array_get_short(da, idx, &s0));
        con_err_ifm (s != s0, "s = %d, s0 = %d, idx = %zu", s, s0, idx);
    }

    u_array_free(da);

    return 0;
err:
    u_array_free(da);

    return -1;
}

static int test_ptr (void)
{
    int rc = 0;
    u_array_t *da = NULL;
    size_t idx;
    struct S { int i; char c; } s, *s0;

    con_err_if (u_array_create(U_ARRAY_TYPE_PTR, 10, &da));

    for (idx = 0; idx < 100; idx++)
    {
        s.i = (int) idx;
        s.c = (char) idx;

        /* explicitly ignore "old values" */
        (void) u_array_set_ptr(da, idx, &s, &rc);
        con_err_ifm (rc == -1, "setting %p at idx %zu failed", &s, idx);

        s0 = u_array_get_ptr(da, idx, &rc);
        con_err_ifm (rc == -1, "getting from idx %zu failed", idx);

        con_err_ifm (s.i != s0->i, "%d != %d at index %zu", s.i, s0->i, idx);
        con_err_ifm (s.c != s0->c, "%c != %c at index %zu", s.c, s0->c, idx);
    }

    u_array_free(da);

    return 0;
err:
    u_array_free(da);

    return -1;
}

static int test_u_short (void)
{
    u_array_t *da = NULL;
    size_t idx;
    unsigned short s, s0;

    con_err_if (u_array_create(U_ARRAY_TYPE_U_SHORT, USHRT_MAX + 1, &da));

    for (s = 0, idx = 0; s < USHRT_MAX; s++, idx++)
    {
        con_err_if (u_array_set_u_short(da, idx, s, NULL));
        con_err_if (u_array_get_u_short(da, idx, &s0));
        con_err_ifm (s != s0, "s = %d, s0 = %d, idx = %zu", s, s0, idx);
    }

    u_array_free(da);

    return 0;
err:
    u_array_free(da);

    return -1;
}

static int test_char (void)
{
    u_array_t *da = NULL;
    size_t idx;
    char s, s0;

    con_err_if (u_array_create(U_ARRAY_TYPE_CHAR, CHAR_MAX * 2 + 1, &da));

    for (s = CHAR_MIN, idx = 0; s < CHAR_MAX; s++, idx++)
    {
        con_err_if (u_array_set_char(da, idx, s, NULL));
        con_err_if (u_array_get_char(da, idx, &s0));
        con_err_ifm (s != s0, "s = %d, s0 = %d, idx = %zu", s, s0, idx);
    }

    u_array_free(da);

    return 0;
err:
    u_array_free(da);

    return -1;
}

static int test_u_char (void)
{
    size_t idx;
    u_array_t *da = NULL;
    unsigned char s, s0;

    con_err_if (u_array_create(U_ARRAY_TYPE_U_CHAR, UCHAR_MAX + 1, &da));

    for (s = 0, idx = 0; s < UCHAR_MAX; s++, idx++)
    {
        con_err_if (u_array_set_u_char(da, idx, s, NULL));
        con_err_if (u_array_get_u_char(da, idx, &s0));
        con_err_ifm (s != s0, "s = %d, s0 = %d, idx = %zu", s, s0, idx);
    }

    u_array_free(da);

    return 0;
err:
    u_array_free(da);

    return -1;
}

U_TEST_SUITE( array )
{
    U_TEST_CASE_ADD( test_ptr );
    U_TEST_CASE_ADD( test_short );
    U_TEST_CASE_ADD( test_u_short );
    U_TEST_CASE_ADD( test_char );
    U_TEST_CASE_ADD( test_u_char );
    U_TEST_CASE_ADD( test_resize );

    return 0;                                                
}
