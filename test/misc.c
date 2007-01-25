#include <u/libu.h>

static int test_u_write(void)
{
    dbg_err("not impl");

    return 0; /* test ok */
err:
    return ~0;  /* test failed */
}

static int test_u_read(void)
{
    int fd;

    dbg_err("not impl");

    return 0; /* test ok */
err:
    return ~0;  /* test failed */
}

static int test_success(void)
{
    return 0;
}

U_TEST_MODULE(misc)
{
    U_TEST_RUN( test_u_read );
    U_TEST_RUN( test_u_write );
    U_TEST_RUN( test_success );

    return 0;                                                
}
