#include <sys/mman.h>
#include <u/libu.h>
#include "blocks.h"

/* 
 * - One (default) or many (non upper bounded) blocks (BLOCKS_OPT_GROW);
 * - Blocks cannot be coalesced, but block size can change at any time during
 *   blocks lifetime, to handle bigger than expected memory needs (i.e. 
 *   blocks->blk_sz could be less than block->sz when requested allocation 
 *   len is bigger than blocks->blk_sz)
 * - ...
 */

struct block_s
{
    size_t sz;                  /* Size of this memory block. */
    size_t offset;              /* Unused memory offset in block. */
    LIST_ENTRY(block_s) next;   /* Next block in chain, if any. */
    unsigned char *mem;         /* mmap'd memory area */
};

typedef struct block_s block_t;

struct blocks_s
{
    size_t cur_sz;  /* Current cumulative size of block. */
    size_t blk_sz;  /* Default size of each mmap'd memory block. */
    unsigned char opts;
    LIST_HEAD(BH, block_s) blocks;
};

static int block_new (size_t sz, block_t **pblk);
static int block_append (block_t *blk, blocks_t *blks);
static void block_free (block_t *blk);
static int block_clear (block_t *blk);

static int round_sz (size_t hint_sz, size_t *psz);
static block_t *first_fit (struct BH *bh, size_t len);

/**
 *  \{
 */ 

/**
 *  \brief  ...
 */
int blocks_new (size_t hint_sz, unsigned char opts, blocks_t **pblks)
{
    size_t blk_sz;
    block_t *blk = NULL;
    blocks_t *blks = NULL;

    dbg_return_if (pblks == NULL, ~0);

    /* Make sensible use of the supplied "hinted" size. */
    dbg_err_if (round_sz(hint_sz, &blk_sz));

    /* Create header. */
    dbg_err_sif ((blks = u_malloc(sizeof *blks)) == NULL);

    /* Create first (and possibly unique) mem block. */
    dbg_err_if (block_new(blk_sz, &blk));

    /* Attach block to header. */
    dbg_err_if (block_append(blk, blks));
    blk = NULL;

    /* Fix header info. */
    blks->opts = opts;
    blks->blk_sz = blks->cur_sz = blk_sz;

    /* Return. */
    *pblks = blks;

    return 0;
err:
    if (blk)
        block_free(blk);
    if (blks)
        blocks_free(blks);

    return ~0;
}

/**
 *  \brief  ...
 */
void *blocks_alloc (blocks_t *blks, size_t len)
{
    void *p;
    size_t sz;
    block_t *blk, *nblk = NULL;

    dbg_return_if (blks == NULL, NULL);
    dbg_return_if (len == 0, NULL); /* 0-sized alloc is not allowed. */

    /* Try to get the first block that fits the requested allocation. */
    if ((blk = first_fit(&blks->blocks, len)) != NULL)
    {
        /* We've got a match from the currently allocated blocks' list:
         *  - save pointer;
         *  - increment offset;
         *  - return saved pointer. */ 
        p = blk->mem + blk->offset;
        blk->offset += len;
        return p;
    }
    else if (!(blks->opts & BLOCKS_OPT_GROW))
    {
        /* The block didn't fit the request, and we're not allowed to grow. */
        return NULL;
    }

    /* See if we need to make a block bigger than default. */
    if (len > blks->blk_sz)
        (void) round_sz(len, &sz);
    else
        sz = blks->blk_sz;

    /* Create another block, append to the blocks' list, and return memory 
     * from it. */
    dbg_err_if (block_new(sz, &nblk));
    p = nblk->mem;
    nblk->offset += len;
    dbg_err_if (block_append(nblk, blks));
    nblk = NULL;

    return p;
err:
    if (nblk)
        block_free(blk);

    return NULL;
}
 
/**
 *  \brief  ...
 */
void blocks_free (blocks_t *blks)
{
    block_t *blk;

    nop_return_if (blks == NULL, );

    LIST_FOREACH (blk, &blks->blocks, next)
        block_free(blk);

    u_free(blks);

    return;
}

/**
 *  \brief  ...
 */
int blocks_clear (blocks_t *blks)
{
    block_t *blk;

    dbg_return_if (blks == NULL, ~0);

    LIST_FOREACH (blk, &blks->blocks, next)
        block_clear(blk);

    return 0;
}

/**
 *  \}
 */ 

static int block_new (size_t sz, block_t **pblk)
{
    block_t *blk = NULL;

    dbg_return_if (pblk == NULL, ~0);
    dbg_return_if (sz == 0, ~0);

    dbg_err_sif ((blk = u_malloc(sizeof *blk)) == NULL);

    blk->mem = mmap(0, sz, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
    dbg_err_sif (blk->mem == MAP_FAILED);

    blk->sz = sz;
    blk->offset = 0;
    LIST_NEXT(blk, next) = NULL;

    *pblk = blk;

    return 0;
err:
    if (blk)
        u_free(blk);

    return ~0;
}

static void block_free (block_t *blk)
{
    nop_return_if (blk == NULL, );

    if (blk->mem != NULL)
        munmap(blk->mem, blk->sz);
    u_free(blk);

    return;
}

static int block_clear (block_t *blk)
{
    dbg_return_if (blk == NULL, ~0);

    blk->offset = 0;  

    return 0;
}

static int block_append (block_t *blk, blocks_t *blks)
{
    dbg_return_if (blk == NULL, ~0);
    dbg_return_if (blks == NULL, ~0);

    LIST_INSERT_HEAD(&blks->blocks, blk, next);

    return 0;
}

static int round_sz (size_t hint_sz, size_t *psz)
{
    /* See ringbuffer... */
    size_t pg_sz = sysconf(_SC_PAGE_SIZE);

    /* Round to the nearest page size multiple. */ 
    *psz = (((hint_sz - 1) / pg_sz) + 1) * pg_sz;

    return 0;
}

static block_t *first_fit (struct BH *bh, size_t len)
{
    block_t *blk;

    LIST_FOREACH (blk, bh, next)
    {
        if ((blk->sz - blk->offset) >= len)
            return blk;
    }

    return NULL;
}
