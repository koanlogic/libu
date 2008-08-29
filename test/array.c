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

U_TEST_MODULE(array);

static int test_resize (void);
static int test_short (void);
static int test_custom (void);
static int test_ushort (void);
static int test_char (void);
static int test_uchar (void);

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

static int test_custom (void)
{
    u_array_t *da = NULL;
    size_t idx;
    struct S { int i; char c; } s, *s0;

    con_err_if (u_array_create(U_ARRAY_TYPE_CUSTOM, 10, &da));

    for (idx = 0; idx < 100; idx++)
    {
        s.i = (int) idx;
        s.c = (char) idx;

        con_err_if (u_array_set_custom(da, idx, (void *) &s, NULL));
        con_err_if (u_array_get_custom(da, idx, (void **) &s0));

        con_err_if (s.i != s0->i);
        con_err_if (s.c != s0->c);
    }

    u_array_free(da);

    return 0;
err:
    u_array_free(da);

    return -1;
}

static int test_ushort (void)
{
    u_array_t *da = NULL;
    size_t idx;
    unsigned short s, s0;

    con_err_if (u_array_create(U_ARRAY_TYPE_USHORT, USHRT_MAX + 1, &da));

    for (s = 0, idx = 0; s < USHRT_MAX; s++, idx++)
    {
        con_err_if (u_array_set_ushort(da, idx, s, NULL));
        con_err_if (u_array_get_ushort(da, idx, &s0));
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

static int test_uchar (void)
{
    u_array_t *da = NULL;
    size_t idx;
    unsigned char s, s0;

    con_err_if (u_array_create(U_ARRAY_TYPE_UCHAR, UCHAR_MAX + 1, &da));

    for (s = 0, idx = 0; s < UCHAR_MAX; s++, idx++)
    {
        con_err_if (u_array_set_uchar(da, idx, s, NULL));
        con_err_if (u_array_get_uchar(da, idx, &s0));
        con_err_ifm (s != s0, "s = %d, s0 = %d, idx = %zu", s, s0, idx);
    }

    u_array_free(da);

    return 0;
err:
    u_array_free(da);

    return -1;
}

U_TEST_MODULE( array )
{
    U_TEST_RUN( test_custom );
    U_TEST_RUN( test_short );
    U_TEST_RUN( test_ushort );
    U_TEST_RUN( test_char );
    U_TEST_RUN( test_uchar );
    U_TEST_RUN( test_resize );

    return 0;                                                
}
