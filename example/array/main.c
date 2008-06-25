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

int main (void)
{
    int i;
    size_t j;
    enum { A_INITIAL_SZ = 3, ADD_SOME_OTHER = 2 } ;
    u_array_t *a = NULL;
    data_t *d = NULL;

    con_err_if (u_array_create(A_INITIAL_SZ, &a));

    /* 
     * load first slot (which fits in original allocation) 
     */
    for (i = 0; i < A_INITIAL_SZ; ++i)
    {
        con_err_if (data_new(i, &d));
        con_err_if (u_array_push(a, (void *) d));
        d = NULL;
    }

    /* dump dynarray contents */
    for (j = 0; j < u_array_top(a); ++j)
        data_print((data_t *) u_array_get_n(a, j));


    con("%zu pushed, now adding other %zu elements (force resize)", 
            A_INITIAL_SZ, ADD_SOME_OTHER);
    /* 
     * add more elements (force resize) 
     */
    for (i = A_INITIAL_SZ; i <= A_INITIAL_SZ + ADD_SOME_OTHER; ++i)
    {
        con_err_if (data_new(i, &d));
        con_err_if (u_array_push(a, (void *) d));
    }

    /* again, dump dynarray contents */
    for (j = 0; j < u_array_top(a); ++j)
        data_print((data_t *) u_array_get_n(a, j));

    /* delete data slots and dyn array */
    for (j = 0; j < u_array_top(a); ++j)
        data_free((data_t *) u_array_get_n(a, j));
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
