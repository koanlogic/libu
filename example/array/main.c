#include <u/libu.h>

int facility = LOG_LOCAL0;

int main (void)
{
    u_array_t *a = NULL;

    con_err_if (u_array_create(0, &a));

    /* insert at high locations to force auto resize */
    con_err_if (u_array_set_n(a, 7, (void *) 7, NULL));
    con_err_if (u_array_set_n(a, 10, (void *) 10, NULL));
    con_err_if (u_array_set_n(a, 30, (void *) 30, NULL));
    /* insert at current array bound (99) */
    con_err_if (u_array_set_n(a, 99, (void *) 99, NULL));

    con("array top is at %zu, array size is %zu", 
            u_array_top(a), u_array_nslot(a));

    /* insert after current array bounds: force += 100 slot increment  */
    con_err_if (u_array_set_n(a, 102, (void *) 102 , NULL));
    con_err_if (u_array_push(a, (void *) 103));

    con("array top is at %zu, array size is %zu", 
            u_array_top(a), u_array_nslot(a));

    u_array_free(a);

    return 0;
err:
    return 1;
}
