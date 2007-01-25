#include <u/libu.h>

static int test_hmap_feature(void)
{
    dbg_err("not impl");

    return 0;
err:
    return ~0;
}

U_TEST_MODULE(hmap)
{
    U_TEST_RUN( test_hmap_feature );

    return 0;
}
