#include <u/libu.h>
#include "blocks.h"

int facility = LOG_LOCAL0;

struct alloc_tv_s
{
    size_t blk_sz, alloc_sz, count;
    ssize_t exp_fail;
    const char *id;
} tv_fixed[] = {
    { 4096, 10, 411, 410, "One block, tiny allocs (10 bytes each)" },
    { 4096, 1, 4096, -1, "One block, micro allocs (1 byte each)" }
}, tv_grow[] = {
    { 4096, 10, 1000, -1, "10 * 1K allocs, auto grow" }
};

static int test_alloc (size_t blk_sz, size_t alloc_sz, size_t count, 
        unsigned char opts, ssize_t exp_fail);
static int test_grow (size_t blk_sz, size_t alloc_sz, size_t count, 
        ssize_t exp_fail);
static int test_fixed (size_t blk_sz, size_t alloc_sz, size_t count,
        ssize_t exp_fail);
static void explain_tv (struct alloc_tv_s *T);
static void br (void);

int main (void)
{
    size_t i;
    struct alloc_tv_s *F, *G;

    for (i = 0; i < sizeof(tv_fixed) / sizeof(struct alloc_tv_s); i++)
    {
        explain_tv((F = &tv_fixed[i]));
        con_err_if (test_fixed(F->blk_sz, F->alloc_sz, F->count, F->exp_fail));
    }

    for (i = 0; i < sizeof(tv_grow) / sizeof(struct alloc_tv_s); i++)
    {
        explain_tv((G = &tv_grow[i]));
        con_err_if (test_grow(G->blk_sz, G->alloc_sz, G->count, G->exp_fail));
    }

    return EXIT_SUCCESS;
err:
    return EXIT_FAILURE;
}

static void explain_tv (struct alloc_tv_s *T)
{
    u_con("\nTest: %s\n"
          "  block size: %u\n"
          "  size of each allocation: %u\n"
          "  number of allocations: %u",
          T->id, T->blk_sz, T->alloc_sz, T->count);
    return;
}

static int test_fixed (size_t blk_sz, size_t alloc_sz, size_t count,
        ssize_t exp_fail)
{
    return test_alloc(blk_sz, alloc_sz, count, BLOCKS_OPT_NONE, exp_fail);
}

static int test_grow (size_t blk_sz, size_t alloc_sz, size_t count, 
        ssize_t exp_fail)
{
    return test_alloc(blk_sz, alloc_sz, count, BLOCKS_OPT_GROW, exp_fail);
}

static int test_alloc (size_t blk_sz, size_t alloc_sz, size_t count, 
        unsigned char opts, ssize_t exp_fail)
{
    size_t i;
    unsigned char *p;
    blocks_t *blks;

    con_err_if (blocks_new(blk_sz, opts, &blks));
    
    for (i = 1; i <= count; i++)
    {
        p = blocks_alloc(blks, alloc_sz);
        //u_con("[%u] %p = block_alloc(%p, %u)", i, p, blks, alloc_sz);
        dbg_goto_if (p == NULL, end);
        memset(p, 'X', alloc_sz);
    }

end: 
err:
    (void) blocks_info(blks);

    if (blks)
        blocks_free(blks);

    return (exp_fail == -1) 
        ? ((i != count + 1) ? ~0 : 0) 
        : ((((size_t) exp_fail != i)) ? ~0 : 0);
}

static void br (void) { return; }
