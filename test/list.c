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

U_TEST_MODULE(list);

static int test_list_ins (void)
{
    enum { ITERS = 3 };
    u_list_t *l = NULL;
    int i, prev;

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

    con_err_if(u_list_del_n(l, 0, (void**)&prev));
    con_err_if(prev != 99);

    con_err_if(u_list_del_n(l, u_list_count(l)-1, (void**)&prev));
    con_err_if(prev != 99);

    for(i = 0; i < ITERS; ++i)
        con_err_if(u_list_insert(l, (void*)99, 2));
    for(i = 0; i < ITERS; ++i)
        con_err_if(u_list_del(l, (void*)99));

    for(i = 0; i < ITERS; ++i)
        con_err_if(u_list_insert(l, (void*)99, 2));
    for(i = 0; i < ITERS; ++i)
    {
        con_err_if(u_list_del_n(l, 2, (void**)&prev));
        con_err_if(prev != 99);
    }

    for(i = 0; i < u_list_count(l); ++i)
        con_err_if(i != (int)u_list_get_n(l, i));

    u_list_free(l);

    return 0;
err:
    return ~0;
}

U_TEST_MODULE( list )
{
    U_TEST_RUN( test_list_ins );

    return 0;                                                
}
