#include <u/libu.h>
#include "test.h"

static int test_hmap_feature(void)
{
    dbg_err("not impl");

    return 0;
err:
    return ~0;
}

TEST_MODULE(hmap)
{
    int rc = 0;

    RUN_TEST( test_hmap_feature );

    return rc;                                                
}
