#include <u/libu.h>
#include "blocks.h"

int facility = LOG_LOCAL0;

static int test_fixed_0 (size_t tot, size_t part, size_t count);
static int test_grow_0 (size_t tot, size_t part, size_t count);

int main (void)
{
    //con_err_if (test_fixed_0(4096, 10, 1000));
    con_err_if (test_grow_0(100, 10, 1000));

    return EXIT_SUCCESS;
err:
    return EXIT_FAILURE;
}

static int test_fixed_0 (size_t tot, size_t part, size_t count)
{
    size_t i;
    unsigned char *p;
    blocks_t *blks;

    con_err_if (blocks_new(tot, BLOCKS_OPT_NONE, &blks));
    
    for (i = 0; i < count; i++)
    {
        p = blocks_alloc(blks, part);
        u_con("[%u] %p = block_alloc(%p, %u)", i, p, blks, part);
        dbg_goto_if (p == NULL, err);
        memset(p, 'X', part);
    }

    return 0;
err:
    return ~0;
}

static int test_grow_0 (size_t tot, size_t part, size_t count)
{
    size_t i;
    unsigned char *p;
    blocks_t *blks;

    con_err_if (blocks_new(tot, BLOCKS_OPT_GROW, &blks));
    
    for (i = 0; i < count; i++)
    {
        p = blocks_alloc(blks, part);
        u_con("[%u] %p = block_alloc(%p, %u)", i, p, blks, part);
        dbg_goto_if (p == NULL, err);
        memset(p, 'X', part);
    }

    return 0;
err:
    return ~0;
}
