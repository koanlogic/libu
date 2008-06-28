#include <u/libu.h>

int facility = LOG_LOCAL0;

int main (void)
{
    u_array_t *a = NULL;

    con_err_if (u_array_create(5, &a));

    /* insert at high locations to force auto resize */
    con_err_if (u_array_set_n(a, 7, (void *) 7, NULL));
    con_err_if (u_array_set_n(a, 10, (void *) 10, NULL));
    con_err_if (u_array_set_n(a, 30, (void *) 30, NULL));

    /* dump data */
    u_array_print(a);

    u_array_free(a);

    return 0;
err:
    return 1;
}
