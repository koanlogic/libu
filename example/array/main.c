#include <u/libu.h>

int facility = LOG_LOCAL0;

int main (void)
{
    u_array_t *a = NULL;

    con_err_if (u_array_create(10, &a));
    con_err_if (u_array_set_n(a, 10000, (void *) 1234, NULL));
    u_array_free(a);

    return 0;
err:
    return 1;
}
