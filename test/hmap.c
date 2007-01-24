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
    RUN_TEST( test_hmap_feature );

    return 0;
}
