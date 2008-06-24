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

U_TEST_MODULE(array);

static int test_u_array_add_get (void)
{
    int *ip, i;
    size_t j;
    u_array_t *a = NULL;

    con_err_if (u_array_create(100, &a));

    /* force resize (4 times) */
    for (i = 0; i < 1000; i++)
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

static int test_u_array_set_get (void)
{
    int *ips = NULL, *ipg = NULL;
    size_t j;
    u_array_t *a = NULL;
    int __i = 5;

    /* create 10-slots array */
    con_err_if (u_array_create(10, &a));

    /* create element that will be inserted at 5th position */
    con_err_sif ((ips = u_zalloc(sizeof(int))) == NULL);
    *ips = 5;

    /* set at 5th slot */
    con_err_if (u_array_set_n(a, &__i, 5));

    /* get at 5th slot */
    ipg = (int *) u_array_get_n(a, 5);
    con_err_if (*ipg != *ips);

    /* try to set at 11th slot (out of bounds) */
    con_err_if (!u_array_set_n(a, ips, 10));

    /* override element at 5th slot (i.e. s/5/6) -- should show up in log  */
    *ips = 6;
    con_err_if (u_array_set_n(a, ips, 5));

    /* get at 5th slot */
    ipg = (int *) u_array_get_n(a, 5);
    con_err_if (*ipg != *ips);

    /* free - since array has sparse values, we shall scan it through 
     * (i.e. use u_array_size instead of u_array_count) */
    for (j = 0; j < u_array_size(a); ++j)
        u_free(u_array_get_n(a, j));
    u_array_free(a);

    return 0;
err:
    return ~0;
}

U_TEST_MODULE( array )
{
    U_TEST_RUN( test_u_array_add_get );
    U_TEST_RUN( test_u_array_set_get );

    return 0;                                                
}
