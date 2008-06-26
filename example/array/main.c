#include <u/libu.h>

int facility = LOG_LOCAL0;

struct data_s
{
    int i;
    char s[1024];
};

typedef struct data_s data_t;

static int data_new (int i, data_t **pd);
static void data_free (data_t *d);
static void data_print (data_t *d);

static int array_of_double (void);
static int array_of_int (void);

int main (void)
{
    con_err_if (array_of_int());
    return 0;
err:
    return 1;
}

static int array_of_int (void)
{
    size_t j;
    int d, d2;
    enum { A_INITIAL_SLOTS = 3, ADD_SOME_OTHER = 2 } ;
    u_array_t *a = NULL;

    con_err_if (u_array_create(sizeof(int), A_INITIAL_SLOTS, &a));

    /* put values at specified locations */
    for (j = 0; j < A_INITIAL_SLOTS; ++j)
    {
        d = (int) j;
        con("put %d at %zu", d, j);
        con_err_if (u_array_set_n(a, j, (void *) d, NULL));
    }

    /* get values and print'em */
    for (j = 0; j < A_INITIAL_SLOTS; ++j)
    {
        con_err_if (u_array_get_n(a, j, (void **) &d2));
        con("get %d at %zu", d2, j);
    }

    /* put another number of values */
    for (j = A_INITIAL_SLOTS; j < A_INITIAL_SLOTS + ADD_SOME_OTHER; ++j)
    {
        d = (int) j;
        con("put %d at %zu", d, j);
        con_err_if (u_array_set_n(a, j, (void *) d, NULL));
    }

    /* get values and print'em */
    for (j = A_INITIAL_SLOTS; j < A_INITIAL_SLOTS + ADD_SOME_OTHER; ++j)
    {
        con_err_if (u_array_get_n(a, j, (void **) &d2));
        con("get %d at %zu", d2, j);
    }

    u_array_free(a);

    return 0;
err:
    return 1;
}

static int array_of_double (void)
{
    size_t j;
    double d, *dp;
    enum { A_INITIAL_SLOTS = 3, ADD_SOME_OTHER = 2 } ;
    u_array_t *a = NULL;

    con_err_if (u_array_create(sizeof(double), A_INITIAL_SLOTS, &a));

    /* put values at specified locations */
    for (j = 0; j < A_INITIAL_SLOTS; ++j)
    {
        d = 0.123456 + (double) j;
        con("put %g at %zu", d, j);
        con_err_if (u_array_set_n(a, j, (void *) &d, NULL));
    }

    /* get values and print'em */
    for (j = 0; j < A_INITIAL_SLOTS; ++j)
    {
        con_err_if (u_array_get_n(a, j, (void **) &dp));
        con("get %g at %zu", *dp, j);
    }

    /* put another number of values */
    for (j = A_INITIAL_SLOTS; j < A_INITIAL_SLOTS + ADD_SOME_OTHER; ++j)
    {
        d = 0.123456 + (double) j;
        con("put %g at %zu", d, j);
        con_err_if (u_array_set_n(a, j, (void *) &d, NULL));
    }

    /* get values and print'em */
    for (j = A_INITIAL_SLOTS; j < A_INITIAL_SLOTS + ADD_SOME_OTHER; ++j)
    {
        con_err_if (u_array_get_n(a, j, (void **) &dp));
        con("get %g at %zu", *dp, j);
    }

    u_array_free(a);

    return 0;
err:
    return 1;
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
