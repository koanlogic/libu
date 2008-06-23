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

static int test_u_array (void)
{
    u_array_t *a = NULL;

    con_err_if (u_array_create(1000, &a));

    /* TODO */

    u_array_free(a);

    return 0;
err:
    return ~0;
}

U_TEST_MODULE( array )
{
    U_TEST_RUN( test_u_array );

    return 0;                                                
}
