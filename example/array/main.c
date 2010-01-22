#include <u/libu.h>

int facility = LOG_LOCAL0;

int main (void)
{
    u_array_t *a = NULL;
    size_t idx;
    long double _Complex c0, c1;

    con_err_if (u_array_create(U_ARRAY_TYPE_LONG_DOUBLE_COMPLEX, 0, &a));

    for (idx = 0; idx < 10; idx++)
    {
        c0 = idx + idx * _Complex_I;
        con_err_if (u_array_set_long_double_complex(a, idx, c0, NULL));
        con_err_if (u_array_get_long_double_complex(a, idx, &c1));
        con_err_if (creal(c0) != creal(c1) || cimag(c0) != cimag(c1));
    }

    for (idx = 0; idx < 10; idx++)
    {
        long double _Complex c2;

        c0 = (idx + 10) + (idx + 10) * _Complex_I;
        con_err_if (u_array_set_long_double_complex(a, idx, c0, &c2));
        u_con("overwrite %lf + %lfi at %zu with %lf + %lfi", 
                creal(c2), cimag(c2), idx, creal(c0), cimag(c0)); 
        con_err_if (u_array_get_long_double_complex(a, idx, &c1));
        con_err_if (creal(c0) != creal(c1) || cimag(c0) != cimag(c1));
    }

    /* index too high */
    con_if (u_array_set_long_double_complex(a, 134217727, c0, NULL));

    u_array_free(a);

    return 0;
err:
    return 1;
}
