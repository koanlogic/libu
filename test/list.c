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

U_TEST_SUITE(list);

static int test_list_iterator(void)
{
    enum { ITERS = 300 };
    u_list_t *l = NULL;
    void *it;
    size_t j;
    intptr_t i, v, tot0, tot1;

    con_err_if(u_list_create(&l));

    con_err_ifm (u_list_count(l), "expecting no items!");

    for (tot0 = 0, i = 1; i < ITERS; ++i)
    {
        con_err_if(u_list_add(l, (void*)i));
        tot0 += i;
    }

    for(i = 1; i < ITERS; ++i)
    {
        con_err_if(u_list_insert(l, (void*)i, i));
        tot0 += i;
    }

    for (tot1 = 0, v = (intptr_t) u_list_first(l, &it); 
            v != 0;
            v = (intptr_t) u_list_next(l, &it))
    {
        tot1 += v;
    }

    con_err_if(tot0 != tot1);

    /* remove some items */
    size_t c = u_list_count(l)/2;
    for(j = 0; j < c; ++j)
    {
        u_list_del_n(l, 0, (void*)&v);
        tot0 -= v;
    }

    for (tot1 = 0, v = (intptr_t) u_list_first(l, &it); 
            v != 0; 
            v = (intptr_t) u_list_next(l, &it))
    {
        tot1 += v;
    }

    con_err_if(tot0 != tot1);

    u_list_free(l);

    return U_TEST_EXIT_SUCCESS;
err:
    return U_TEST_EXIT_FAILURE;
}

static int test_list_ins (void)
{
    enum { ITERS = 3 };
    u_list_t *l = NULL;
    uintptr_t i;
    void* prev;

    con_err_if(u_list_create(&l));
    con_err_if(u_list_add(l, (void*)1));
    con_err_if(u_list_add(l, (void*)2));
    con_err_if(u_list_add(l, (void*)99));
    con_err_if(u_list_add(l, (void*)2));
    con_err_if(u_list_add(l, (void*)4));

    con_err_if(u_list_insert(l, (void*)0, 0));
    con_err_if(u_list_insert(l, (void*)3, 3));
    con_err_if(u_list_del(l, (void*)99));
    con_err_if(u_list_del_n(l, 4, NULL));

    con_err_if(u_list_insert(l, (void*)99, 0));
    con_err_if(u_list_insert(l, (void*)99, u_list_count(l)));

    con_err_if(u_list_del_n(l, 0, &prev));
    con_err_if((uintptr_t) prev != 99);

    con_err_if(u_list_del_n(l, u_list_count(l)-1, &prev));
    con_err_if((int)prev != 99);

    for(i = 0; i < ITERS; ++i)
        con_err_if(u_list_insert(l, (void*)99, 2));
    for(i = 0; i < ITERS; ++i)
        con_err_if(u_list_del(l, (void*)99));

    for(i = 0; i < ITERS; ++i)
        con_err_if(u_list_insert(l, (void*)99, 2));

    for (i = 0; i < ITERS; ++i)
    {
        con_err_if (u_list_del_n(l, 2, &prev));
        con_err_if ((uintptr_t) prev != 99);
    }

    for (i = 0; i < (uintptr_t) u_list_count(l); ++i)
        con_err_if (i != (uintptr_t) u_list_get_n(l, i));

    u_list_free(l);

    return U_TEST_EXIT_SUCCESS;
err:
    if (l)
        u_list_free(l);

    return U_TEST_EXIT_FAILURE;
}

U_TEST_SUITE( list )
{
    U_TEST_CASE_ADD( test_list_ins );
    U_TEST_CASE_ADD( test_list_iterator );

    return 0;                                                
}
