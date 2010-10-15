#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <u/libu.h>

int test_suite_list_register (u_test_t *t);

static int test_list_iterator (u_test_case_t *tc);
static int test_list_ins (u_test_case_t *tc);

static int test_list_iterator (u_test_case_t *tc)
{
    enum { ITERS = 300 };
    u_list_t *l = NULL;
    void *it;
    size_t j, c;
    intptr_t i, v, tot0, tot1;

    u_test_err_if (u_list_create(&l));

    u_test_err_ifm (u_list_count(l), "expecting no items!");

    for (tot0 = 0, i = 1; i < ITERS; ++i)
    {
        u_test_err_if (u_list_add(l, (void*)i));
        tot0 += i;
    }

    for (i = 1; i < ITERS; ++i)
    {
        u_test_err_if (u_list_insert(l, (void*)i, i));
        tot0 += i;
    }

    for (tot1 = 0, v = (intptr_t) u_list_first(l, &it); 
            v != 0;
            v = (intptr_t) u_list_next(l, &it))
    {
        tot1 += v;
    }

    u_test_err_if (tot0 != tot1);

    /* remove some items */
    c = u_list_count(l)/2;
    for (j = 0; j < c; ++j)
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

    u_test_err_if (tot0 != tot1);

    u_list_free(l);

    return U_TEST_SUCCESS;
err:
    return U_TEST_FAILURE;
}

static int test_list_ins (u_test_case_t *tc)
{
    enum { ITERS = 3 };
    u_list_t *l = NULL;
    uintptr_t i;
    void* prev;

    u_test_err_if (u_list_create(&l));
    u_test_err_if (u_list_add(l, (void*)1));
    u_test_err_if (u_list_add(l, (void*)2));
    u_test_err_if (u_list_add(l, (void*)99));
    u_test_err_if (u_list_add(l, (void*)2));
    u_test_err_if (u_list_add(l, (void*)4));

    u_test_err_if (u_list_insert(l, (void*)0, 0));
    u_test_err_if (u_list_insert(l, (void*)3, 3));
    u_test_err_if (u_list_del(l, (void*)99));
    u_test_err_if (u_list_del_n(l, 4, NULL));

    u_test_err_if (u_list_insert(l, (void*)99, 0));
    u_test_err_if (u_list_insert(l, (void*)99, u_list_count(l)));

    u_test_err_if (u_list_del_n(l, 0, &prev));
    u_test_err_if ((uintptr_t) prev != 99);

    u_test_err_if (u_list_del_n(l, u_list_count(l)-1, &prev));
    u_test_err_if ((uintptr_t) prev != 99);

    for (i = 0; i < ITERS; ++i)
        u_test_err_if (u_list_insert(l, (void*)99, 2));
    for (i = 0; i < ITERS; ++i)
        u_test_err_if (u_list_del(l, (void*)99));

    for (i = 0; i < ITERS; ++i)
        u_test_err_if (u_list_insert(l, (void*)99, 2));

    for (i = 0; i < ITERS; ++i)
    {
        u_test_err_if  (u_list_del_n(l, 2, &prev));
        u_test_err_if  ((uintptr_t) prev != 99);
    }

    for (i = 0; i < (uintptr_t) u_list_count(l); ++i)
        u_test_err_if  (i != (uintptr_t) u_list_get_n(l, i));

    u_list_free(l);

    return U_TEST_SUCCESS;
err:
    if (l)
        u_list_free(l);

    return U_TEST_FAILURE;
}

int test_suite_list_register (u_test_t *t)
{
    u_test_suite_t *ts = NULL;

    con_err_if (u_test_suite_new("Lists", &ts));

    con_err_if (u_test_case_register("Insertion", test_list_ins, ts));
    con_err_if (u_test_case_register("Iteration", test_list_iterator, ts));

    return u_test_suite_add(ts, t);
err:
    u_test_suite_free(ts);
    return ~0;
}
