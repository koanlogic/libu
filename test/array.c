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
    int *ip, i;
    size_t j;
    u_array_t *a = NULL;
    enum { A_SZ = 100 };

    con_err_if (u_array_create(A_SZ, &a));

    /* force four resize (2^n > 10 <=> n > 3) */
    for (i = 0; i < A_SZ * 10; i++)
    {
        con_err_sif ((ip = u_zalloc(sizeof(int))) == NULL);
        con_err_if (u_array_push(a, (void *) ip));
        ip = NULL;
    }

    con("total number of slots: %zu", u_array_size(a));
    con("used slots: %zu", u_array_count(a));
    con("slots still available: %zu", u_array_avail(a));

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

static int test_u_array_set_get (void)
{
    int *ips = NULL, *ipg = NULL;
    size_t j;
    u_array_t *a = NULL;
    enum { A_SZ = 10 };

    /* create 10-slots array */
    con_err_if (u_array_create(A_SZ, &a));

    /* create element that will be inserted at a median position */
    con_err_sif ((ips = u_zalloc(sizeof(int))) == NULL);
    *ips = 5;

    /* set at median slot */
    con_err_if (u_array_set_n(a, ips, A_SZ / 2));

    /* get at median slot */
    ipg = (int *) u_array_get_n(a, A_SZ / 2);
    con_err_if (*ipg != *ips);

    /* try to set off-by-one */
    con_err_if (!u_array_set_n(a, ips, A_SZ));

    /* override element at median slot (i.e. s/5/6) -- should show up in log  */
    *ips = 6;
    con_err_if (u_array_set_n(a, ips, A_SZ / 2));

    /* get at median slot */
    ipg = (int *) u_array_get_n(a, A_SZ / 2);
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
