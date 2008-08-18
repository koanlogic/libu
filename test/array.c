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

#define PR_HEADER   con("\n((%s))", __FUNCTION__);

struct data_s
{
    int i;
    char s[1024];
};

typedef struct data_s data_t;

static int data_new (int i, data_t **pd);
static void data_free (data_t *d);
static void data_print (data_t *d);
static void __data_free (void *d);

static int scalar_int_array (void);
static int ptr_double_array (void);
static int ptr_custom_array (void);


/* with int's and in general when dealing with scalar types whose size is less
 * then sizeof(void *) - i.e. 4 bytes - we can simply push the scalar value
 * at a specific location, e.g.: 
 *      u_array_set_n(a, 3, (void *) 1234)) 
 */
static int scalar_int_array (void)
{
    size_t j;
    int d, d2;
    enum { A_INITIAL_SLOTS = 3, ADD_SOME_OTHER = 2, TOT_ELEM = 10 } ;
    u_array_t *a = NULL;

    PR_HEADER

    con_err_if (u_array_create(A_INITIAL_SLOTS, &a));

    /* 
     * put/get at slots 0..A_INITIAL_SLOTS-1 
     */ 
    for (j = 0; j < A_INITIAL_SLOTS; ++j)
    {
        d = (int) j;
        con("put %d at [%zu]", d, j);

        con_err_if (u_array_set_n(a, j, (void *) d, NULL));
    }

    for (j = 0; j < A_INITIAL_SLOTS; ++j)
    {
        con_err_if (u_array_get_n(a, j, (void **) &d2));

        con("  got %d at [%zu] : <%s>", d2, j, (d2 != (int) j) ? "ko" : "ok");
    }

    /* 
     * put/get at slots A_INITIAL_SLOTS..A_INITIAL_SLOTS+ADD_SOME_OTHER-1 
     */
    for (j = A_INITIAL_SLOTS; j < A_INITIAL_SLOTS + ADD_SOME_OTHER; ++j)
    {
        d = (int) j;
        con("put %d at [%zu]", d, j);

        con_err_if (u_array_set_n(a, j, (void *) d, NULL));
    }

    for (j = A_INITIAL_SLOTS; j < A_INITIAL_SLOTS + ADD_SOME_OTHER; ++j)
    {
        con_err_if (u_array_get_n(a, j, (void **) &d2));

        con("  got %d at [%zu] : <%s>", d2, j, (d2 != (int) j) ? "ko" : "ok");
    }

    /* 
     * push cycle (force resize)
     */
    for (j = A_INITIAL_SLOTS + ADD_SOME_OTHER; j < TOT_ELEM; ++j)
    {
        d = (int) j;

        con_err_if (u_array_push(a, (void *) d));

        con("push %d on top (%zu)", d, j, u_array_top(a));
    }

    u_array_free(a);

    return 0;
err:
    u_array_free(a);
    return ~0;
}

/* sizeof(double) is 8 (i.e. > sizeof(void *)), so we are forced to use the
 * put/get by pointer strategy.
 */
static int ptr_double_array (void)
{
#define F   0.1234
    size_t j;
    double *d, *dp;
    enum { A_INITIAL_SLOTS = 3, ADD_SOME_OTHER = 2 } ;
    u_array_t *a = NULL;

    PR_HEADER

    con_err_if (u_array_create(A_INITIAL_SLOTS, &a));
    con_err_if (u_array_set_cb_free(a, u_free));

    /* 
     * put/get at slots 0..A_INITIAL_SLOTS-1 
     */ 
    for (j = 0; j < A_INITIAL_SLOTS; ++j)
    {
        con_err_if ((d = u_zalloc(sizeof(double))) == NULL);
        *d = F + (double) j;
        con("put %g at [%zu]", *d, j);

        con_err_if (u_array_set_n(a, j, d, NULL));
    }

    for (j = 0; j < A_INITIAL_SLOTS; ++j)
    {
        con_err_if (u_array_get_n(a, j, (void **) &dp));

        con("  got %g at [%zu] : <%s>", 
                *dp, j, ((*dp - F) != (double) j) ? "ko" : "ok");
    }

    /* 
     * put/get at slots A_INITIAL_SLOTS..A_INITIAL_SLOTS+ADD_SOME_OTHER-1 
     */
    for (j = A_INITIAL_SLOTS; j < A_INITIAL_SLOTS + ADD_SOME_OTHER; ++j)
    {
        con_err_if ((d = u_zalloc(sizeof(double))) == NULL);
        *d = F + (double) j;
        con("put %g at [%zu]", *d, j);

        con_err_if (u_array_set_n(a, j, d, NULL));
    }

    for (j = A_INITIAL_SLOTS; j < A_INITIAL_SLOTS + ADD_SOME_OTHER; ++j)
    {
        con_err_if (u_array_get_n(a, j, (void **) &dp));

        con("  got %g at [%zu] : <%s>", 
                *dp, j, ((*dp - F) != (double) j) ? "ko" : "ok");
    }

    u_array_free(a);

    return 0;
err:
    u_array_free(a);
    return ~0;
#undef F
}

static int ptr_custom_array (void)
{
    size_t j;
    data_t *d = NULL, *d2, *drepl = NULL, *dnew;
    u_array_t *a = NULL;
    enum { A_INITIAL_SLOTS = 3, ADD_SOME_OTHER = 2 } ;

    PR_HEADER

    con_err_if (u_array_create(A_INITIAL_SLOTS, &a));
    con_err_if (u_array_set_cb_free(a, __data_free));

    for (j = 0; j < A_INITIAL_SLOTS + ADD_SOME_OTHER; ++j)
    {
        con_err_if (data_new(j, &d));

        con_err_if (u_array_set_n(a, j, d, NULL));
    }

    for (j = 0; j < A_INITIAL_SLOTS + ADD_SOME_OTHER; ++j)
    {
        con_err_if (u_array_get_n(a, j, (void **) &d2));

        data_print(d2);
    }

    /* replace an already set slot (idx=3) */
    con_err_if (data_new(9999, &dnew));
    con_err_if (u_array_set_n(a, A_INITIAL_SLOTS, dnew, (void **) &drepl));
    /* free the replaced data (otherwise it would be lost) */
    data_free(drepl);

    /*
     * print whole array content
     */
    for (j = 0; j <= u_array_top(a); ++j)
    {
        con_err_if (u_array_get_n(a, j, (void **) &d2));

        data_print(d2);
    }

    u_array_free(a);

    return 0;
err:
    u_array_free(a);
    return ~0;
}

static int data_new (int i, data_t **pd)
{
    data_t *d = NULL;

    con_return_if (pd == NULL, ~0);

    con_return_sif ((d = u_zalloc(sizeof(data_t))) == NULL, ~0);

    d->i = i;
    u_snprintf(d->s, sizeof d->s, "%d", i);

    *pd = d;

    return 0;
}

static void __data_free (void *d)
{
    data_free((data_t *) d);
    return;
}

static void data_free (data_t *d)
{
    U_FREE(d);
    return;
}

static void data_print (data_t *d)
{
    if (d == NULL)
        con("d(<NULL>)");
    else
    {
        con("d(%p)", d);
        con("   .i = %u", d->i);
        con("   .s = \'%s\'", d->s);
    }

    return;
}

static int push_bug_test (void)
{
    u_array_t *a = NULL;
    void *ptr;

    PR_HEADER

    con_err_if (u_array_create(0, &a));

    /* should go into the slot number zero... */
    con_err_if (u_array_push(a, 0xdeadbeef));

    con_err_if (u_array_get_n(a, 0, &ptr));

    con_err_if (ptr != 0xdeadbeef);

    u_array_free(a);

    return 0;
err:
    u_array_free(a);
    return ~0;
}

U_TEST_MODULE( array )
{
    U_TEST_RUN( scalar_int_array );
    U_TEST_RUN( ptr_double_array );
    U_TEST_RUN( ptr_custom_array );
    U_TEST_RUN( push_bug_test );

    return 0;                                                
}
