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

static int test_u_array_sparse (void)
{
    int odd, *ips = NULL;
    size_t j, even;
    u_array_t *a = NULL;
    enum { A_SZ = 10 };

    con_err_if (u_array_create(A_SZ, &a));
    
    /* fill even slots with the corresponding int */
    for (even = 0; even < A_SZ; even += 2)
    {
        con_err_sif ((ips = u_zalloc(sizeof(int))) == NULL);
        *ips = even;

        con_err_if (u_array_set_n(a, ips, even));

        ips = NULL;
    }

    /* fill remaining slots (i.e. odds) with the corresponding int */
    for (odd = 1; u_array_avail(a); odd += 2)
    {
        con_err_sif ((ips = u_zalloc(sizeof(int))) == NULL);
        *ips = odd;

        /* if u_array_avail condition is satisfied, it is safe to use 
         * u_array_lower_free_slot without checking the ret code */
        con_err_if (u_array_set_n(a, ips, u_array_lower_free_slot(a)));

        ips = NULL;
    }

    /* check inserted values (and free'em) */
    for (j = 0; j <= u_array_top(a); ++j)
    {
        int *p = (int *) u_array_get_n(a, j);
        con_err_if ((size_t) *p != j);
        u_free(p);
    }

    u_array_free(a);

    return 0;
err:
    return ~0;
}

static int test_u_array_push_get (void)
{
    int *ip, *p, i;
    size_t j;
    u_array_t *a = NULL;

    con_err_if (u_array_create(100, &a));

    /* force resize (4 times) */
    for (i = 0; i < 1000; i++)
    {
        con_err_sif ((ip = u_zalloc(sizeof(int))) == NULL);
        con_err_if (u_array_push(a, (void *) ip));
        /* caller still owns 'ip' */
    }

    con("total number of slots: %zu", u_array_size(a));
    con("used slots: %zu", u_array_count(a));
    con("slots still available: %zu", u_array_avail(a));

    for (j = 0; j <= u_array_top(a); ++j)
    {
        p = (int *) u_array_get_n(a, j);
        U_FREE(p);
    }
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

    /* create 10-slots array */
    con_err_if (u_array_create(10, &a));

    /* create element that will be inserted at 5th position */
    con_err_sif ((ips = u_zalloc(sizeof(int))) == NULL);
    *ips = 5;

    /* set at 5th slot */
    con_err_if (u_array_set_n(a, ips, 5));

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

    /* free */
    for (j = 0; j <= u_array_top(a); ++j)
    {
        int *p = u_array_get_n(a, j);
        U_FREE(p);
    }
    u_array_free(a);

    return 0;
err:
    return ~0;
}

U_TEST_MODULE( array )
{
    U_TEST_RUN( test_u_array_push_get );
    U_TEST_RUN( test_u_array_set_get );
    U_TEST_RUN( test_u_array_sparse );

    return 0;                                                
}
