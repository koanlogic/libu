/* 
 * (c) KoanLogic Srl 2010
 */

#ifndef _BLOCKS_H_
#define _BLOCKS_H_

struct blocks_s;

enum {
    BLOCKS_OPT_NONE    = 0x00,  /* Default is one big block of fixed size. */
    BLOCKS_OPT_GROW    = 0x01   /* More memory blocks are added if needed. */
};

typedef struct blocks_s blocks_t;

int blocks_new (size_t blk_sz, unsigned char opts, blocks_t **pblks);
void blocks_free (blocks_t *blks);
void *blocks_alloc (blocks_t *blks, size_t len);
int blocks_clear (blocks_t *blks);
int blocks_copyout (blocks_t *blks, void *src, void *dst, size_t nbytes);
int blocks_info (blocks_t *blks);

#endif  /* !_BLOCKS_H_ */
