#include <u/libu.h>
#include "test.h"

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

TEST_MODULE(misc)
{
    int rc = 0;

    RUN_TEST( test_u_read );
    RUN_TEST( test_u_write );
    RUN_TEST( test_success );

    return rc;                                                
}
