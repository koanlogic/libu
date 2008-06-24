#include <string.h>
#include <syslog.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <u/libu.h>

static int test_u_array (void)
{
    int *ip, i;
    size_t j;
    u_array_t *a = NULL;

    con_err_if (u_array_create(100, &a));

    for (i = 0; i < 10000; i++)
    {
        con_err_sif ((ip = u_zalloc(sizeof(int))) == NULL);
        con_err_if (u_array_add(a, (void *) ip));
        /* caller still owns 'ip' */
    }

    con("total number of slots: %zu", u_array_size(a));
    con("used slots: %zu", u_array_count(a));
    con("slots still available: %zu", u_array_avail(a));

    for (j = 0; j < u_array_count(a); ++j)
        u_free((int *) u_array_get_n(a, j));
    u_array_free(a);

    return 0;
err:
    return ~0;
}

U_TEST_MODULE( array )
{
    U_TEST_RUN( test_u_array );

    return 0;                                                
}
