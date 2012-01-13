/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <u/toolbox/rb.h>
#include <u/toolbox/misc.h>
#include <u/toolbox/carpal.h>

#if defined(U_RB_CAN_MMAP)
  #include <sys/mman.h>
#if defined(HAVE_SYSCONF) && defined(_SC_PAGE_SIZE)
  #define u_vm_page_sz  sysconf(_SC_PAGE_SIZE)
#elif defined (HAVE_SYSCONF) && defined(_SC_PAGESIZE)
  #define u_vm_page_sz  sysconf(_SC_PAGESIZE)
#elif defined(HAVE_GETPAGESIZE)
  #define u_vm_page_sz  getpagesize()
#else
  #error "don't know how to get page size.  reconfigure with --no_ringbuffer."
#endif
#endif  /* U_RB_CAN_MMAP */

struct u_rb_s
{
    char *base;     /* base address of the memory buffer. */
    size_t sz;      /* ring buffer size */
    size_t wr_off;  /* write offset */
    size_t rd_off;  /* read offset */
    size_t ready;   /* bytes ready to be read */
    int opts;       /* options */

    /* Implementation specific methods. */
    void (*cb_free) (struct u_rb_s *rb);
    ssize_t (*cb_read) (struct u_rb_s *rb, void *b, size_t b_sz);
    void *(*cb_fast_read) (struct u_rb_s *rb, size_t *pb_sz);
    ssize_t (*cb_write) (struct u_rb_s *rb, const void *b, size_t b_sz);
};

#define read_addr(rb)   (rb->base + rb->rd_off)
#define write_addr(rb)  (rb->base + rb->wr_off)

static void write_incr_contiguous (u_rb_t *rb, size_t cnt);
static void write_incr_wrapped (u_rb_t *rb, size_t cnt);
static void read_incr_wrapped (u_rb_t *rb, size_t cnt);
static void read_incr_contiguous (u_rb_t *rb, size_t cnt);
static ssize_t write_contiguous (u_rb_t *rb, const void *b, size_t b_sz);
static ssize_t write_wrapped (u_rb_t *rb, const void *b, size_t b_sz);
static ssize_t read_contiguous (u_rb_t *rb, void *b, size_t b_sz);
static ssize_t read_wrapped (u_rb_t *rb, void *b, size_t b_sz);
static void *fast_read (u_rb_t *rb, size_t *pb_sz);
static int is_wrapped (u_rb_t *rb);
static int mirror (u_rb_t *rb, const void *b, size_t to_be_written);

#if defined(U_RB_CAN_MMAP)  /* specific to mmap(2) based implementation. */
  static int create_mmap (size_t hint_sz, int opts, u_rb_t **prb);
  static void free_mmap (u_rb_t *rb);
  static size_t round_sz (size_t sz);
#endif  /* U_RB_CAN_MMAP */

static int create_malloc (size_t sz, int opts, u_rb_t **prb);
static void free_malloc (u_rb_t *rb);

/**
    \defgroup rb Ring Buffer
    \{
        The \ref rb module provides a fast implementation of a circular buffer.
        Data can be written to (::u_rb_write), or read (::u_rb_read) 
        from the buffer.  When the buffer fills up (i.e. the reader is less
        efficient that the writer) any subsequent ::u_rb_write operation will
        fail until more space is made available either by an explicit reset of 
        the buffer - which is done via ::u_rb_clear - or by an ::u_rb_read 
        operation.  The ::u_rb_avail and ::u_rb_ready functions can be used
        by the writer and the reader respectively to test if their actions
        can be taken.
        Initialization:
    \code
    size_t rb_sz, to_write, to_read;
    u_rb_t *rb = NULL;

    // create a ring buffer of size (at least) 1024 bytes
    dbg_err_if (u_rb_create(1024, U_RB_OPT_NONE, &rb));

    // you can get the actual size of the buffer by using u_rb_size
    con("ring buffer (@%p) of size %zu", rb, (rb_sz = u_rb_size(rb)));
    \endcode

        The writer:
    \code
    // assuming ibufp is an ideal, infinite size buffer :)
    while ((to_write = u_rb_avail(rb)) > 0)
    {
        (void) u_rb_write(rb, ibufp, to_write);
        ibufp += to_write;
    }
    \endcode
        The reader is actually the dual:
    \code
    // assuming obufp is the same class as ibufp
    while ((to_read = u_rb_ready(rb)) > 0)
    {
        (void) u_rb_read(rb, obufp, to_read);
        obufp += to_read;
    }
    \endcode

    There exist two distinct implementation of the underlying machinery, one
    is malloc(3) based, the other is mmap(2) based.  The latter is the default
    on platforms supporting the mmap syscall.  Anyway, the user can choose to
    use the malloc implementation by supplying ::U_RB_OPT_IMPL_MALLOC as an
    option parameter at creation time.
    The malloc implementation comes in two different flavours: one using a 
    double-size buffer which enables the ::u_rb_fast_read (no-copy) interface, 
    the other using a buffer exactly the size requested by the user, but 
    disallowing the "no-copy-read" interface (this is the default).
    The user can always enable the contiguous mode by supplying the 
    ::U_RB_OPT_USE_CONTIGUOUS_MEM as a ::u_rb_create option.

    \note

    - The \ref rb module is not thread safe: should you need to use it in a MT 
      scenario, you'll have to wrap the ::u_rb_t into a mutexed object and 
      bound the relevant operations to the mutex acquisition.

    - A read interface which minimises overhead (::u_rb_fast_read) is enabled 
      on ::u_rb_t objects created with the ::U_RB_OPT_USE_CONTIGUOUS_MEM flag 
      set (the mmap implementation always uses the contiguous memory strategy).
 */
 
/**
 *  \brief  Create a new ring buffer object
 *
 *  Create a new ::u_rb_t object of size (at least) \p hint_sz and return 
 *  it at \p *prb.  The memory region used to hold the buffer is \c mmap'd 
 *  and as such it has "page alignment" constraint which could make it 
 *  space-inefficient in case a very tiny (i.e. much smaller than the system
 *  page size) ring buffer is needed.  In fact, optimal size for a ::u_rb_t
 *  object is a multiple of the target system page size.
 *
 *  \param  hint_sz the suggested size in bytes for the ring buffer (the actual
 *                  size could be more than that because of alignment needs)
 *  \param  opts    Bitwise inclusive OR of ::u_rb_opts_t values
 *  \param  prb     result argument which holds the reference to the newly
 *                  created ::u_rb_t object
 *
 *  \retval  0  on success
 *  \retval -1  on error
 *
 *  \note   We try to use the mmap-based implementation whenever possible.
 */
int u_rb_create (size_t hint_sz, int opts, u_rb_t **prb)
{
    if (opts & U_RB_OPT_IMPL_MALLOC)
        return create_malloc(hint_sz, opts, prb);

#if defined(U_RB_CAN_MMAP)
    /* Default is to use mmap implementation. */
    return create_mmap(hint_sz, opts, prb);
#else   /* !U_RB_CAN_MMAP */
    /* Fallback to malloc in case mmap is not available. */
    return create_malloc(hint_sz, opts | U_RB_OPT_IMPL_MALLOC, prb);
#endif  /* U_RB_CAN_MMAP */
}

/**
 *  \brief  Dispose a ring buffer object
 *
 *  Dispose the previously allocated ::u_rb_t object \p rb
 *
 *  \param  rb  reference to the ::u_rb_t object that must be disposed
 *
 *  \return nothing
 */
void u_rb_free (u_rb_t *rb)
{
    if (rb && rb->cb_free)
        rb->cb_free(rb);
    return;
}

/**
 *  \brief  Return the size of the ring buffer
 *
 *  Return the real size (as possibly rounded by the ::u_rb_create routine) of 
 *  the supplied ::u_rb_t object \p rb
 *
 *  \param  rb  reference to an already allocated ::u_rb_t object
 *
 *  \return the size in bytes of the ring buffer
 */
size_t u_rb_size (u_rb_t *rb)
{
    return rb->sz;
}

/**
 *  \brief  Write to the ring buffer
 *
 *  Try to write \p b_sz bytes of data from the memory block \p b to the 
 *  ::u_rb_t object \p rb
 *
 *  \param  rb      reference to an already allocated ::u_rb_t object where
 *                  data will be written to
 *  \param  b       reference to the memory block which will be copied-in
 *  \param  b_sz    number of bytes starting from \p b that the caller wants
 *                  to be copied into \p rb
 *
 *  \return the number of bytes actually written to the ring buffer (may be 
 *              less than the requested size)
 */
ssize_t u_rb_write (u_rb_t *rb, const void *b, size_t b_sz)
{
    ssize_t rc;

    dbg_return_if (rb == NULL, -1);
    dbg_return_if (rb->cb_write == NULL, -1); 

    /* Also increment the number of ready bytes. */
    if ((rc = rb->cb_write(rb, b, b_sz)) >= 0)
        rb->ready += (size_t) rc;

    return rc;
}

/**
 *  \brief  Read from the ring buffer
 *
 *  Try to read \p b_sz bytes of data from the ring buffer \p rb and copy it 
 *  to \p b
 *
 *  \param  rb      reference to an already allocated ::u_rb_t object where 
 *                  data will be read from
 *  \param  b       reference to the memory block where data will be copied
 *                  (must be pre-allocated by the caller and of \p b_sz bytes
 *                  at least)
 *  \param  b_sz    number of bytes that the caller wants to be copied into 
 *                  \p b
 *
 *  \return the number of bytes actually read from the ring buffer (may be 
 *              less than the requested size)
 */
ssize_t u_rb_read (u_rb_t *rb, void *b, size_t b_sz)
{
    ssize_t rc;

    dbg_return_if (rb == NULL, -1);
    dbg_return_if (rb->cb_read == NULL, -1); 

    /* Decrement the number of ready bytes. */
    if ((rc = rb->cb_read(rb, b, b_sz)) >= 0)
        rb->ready -= (size_t) rc;

    return rc;
}

/**
 *  \brief  Read from the ring buffer with minimum overhead
 *
 *  Try to read \p *pb_sz bytes of data from the ring buffer \p rb and return
 *  the pointer to the start of data.  The ::U_RB_OPT_USE_CONTIGUOUS_MEM 
 *  option must have been set when creating \p rb.
 *
 *  \param  rb      reference to an already allocated ::u_rb_t object where 
 *                  data will be read from
 *  \param  pb_sz   number of bytes that the caller wants to be read.  On 
 *                  successful return (i.e. non \c NULL) the value will be 
 *                  filled with the number of bytes actually read.
 *
 *  \return the address to the start of read data, or \c NULL on error (which
 *          includes the "no data ready" condition).
 *
 *  \note   In case there is no data ready to be read a \c NULL is returned,
 *          with <code>*pb_sz == 0</code>.  Any other error condition won't 
 *          touch the supplied \p *pb_sz value.
 */
void *u_rb_fast_read (u_rb_t *rb, size_t *pb_sz)
{
    dbg_return_if (rb == NULL, NULL);
    dbg_return_if (rb->cb_write == NULL, NULL);

    return rb->cb_fast_read(rb, pb_sz);
}

/**
 *  \brief  Reset the ring buffer
 *
 *  Reset read and write offset counters of the supplied ::u_rb_t object \p rb
 *
 *  \param  rb      reference to an already allocated ::u_rb_t object
 *
 *  \retval  0  on success
 *  \retval -1  on failure
 */
int u_rb_clear (u_rb_t *rb)
{
    dbg_return_if (rb == NULL, -1);
    rb->wr_off = rb->rd_off = rb->ready = 0;
    return 0;
}
 
/**
 *  \brief  Return the number of bytes ready to be consumed
 *
 *  Return the number of bytes ready to be consumed for the supplied ::u_rb_t
 *  object \p rb
 *
 *  \param  rb      reference to an already allocated ::u_rb_t object 
 *
 *  \return  the number of bytes ready to be consumed (could be \c 0)
 */
size_t u_rb_ready (u_rb_t *rb)
{
    /* Let it crash on NULL 'rb's */
    return rb->ready;
}
 
/**
 *  \brief  Return the unused buffer space
 *
 *  Return number of unused bytes in the supplied ::u_rb_t object, i.e. 
 *  the dual of ::u_rb_ready.
 *
 *  \param  rb      reference to an already allocated ::u_rb_t object 
 *
 *  \return the number of bytes that can be ::u_rb_write'd before old data 
 *          starts being overwritten
 */
size_t u_rb_avail (u_rb_t *rb)
{
    return (rb->sz - rb->ready);
}

/**
 *  \}
 */ 


/* just shift the write pointer */
static void write_incr_contiguous (u_rb_t *rb, size_t cnt)
{
    rb->wr_off += cnt;
}

/* shift the write pointer */
static void write_incr_wrapped (u_rb_t *rb, size_t cnt)
{
    rb->wr_off = (rb->wr_off + cnt) % rb->sz;
}

/* shift the read pointer */
static void read_incr_wrapped (u_rb_t *rb, size_t cnt)
{
    rb->rd_off = (rb->rd_off + cnt) % rb->sz;
}

/* shift the read pointer of 'cnt' positions */
static void read_incr_contiguous (u_rb_t *rb, size_t cnt)
{
    /* 
     * assume 'rb' and 'cnt' sanitized 
     */
    rb->rd_off += cnt;

    /* When the read offset is advanced into the second virtual-memory region, 
     * both offsets - read and write - are decremented by the length of the 
     * underlying buffer */
    if (rb->rd_off >= rb->sz)
    {
        rb->rd_off -= rb->sz;
        rb->wr_off -= rb->sz;
    }

    return;
}

static void *fast_read (u_rb_t *rb, size_t *pb_sz)
{
    void *data = NULL;

    dbg_return_if (rb == NULL, NULL);
    dbg_return_if (!(rb->opts & U_RB_OPT_USE_CONTIGUOUS_MEM), NULL);
    dbg_return_if (pb_sz == NULL, NULL);
    dbg_return_if (*pb_sz > u_rb_size(rb), NULL);

    /* if there is nothing ready to be read go out immediately */
    nop_goto_if (!(*pb_sz = U_MIN(u_rb_ready(rb), *pb_sz)), end);

    data = read_addr(rb);
    read_incr_contiguous(rb, *pb_sz);
    rb->ready -= *pb_sz;

    /* fall through */
end:
    return data;
}

static int is_wrapped (u_rb_t *rb)
{
    return (rb->rd_off > rb->wr_off);
}

static ssize_t write_contiguous (u_rb_t *rb, const void *b, size_t b_sz)
{
    size_t to_be_written;

    dbg_return_if (rb == NULL, -1);
    dbg_return_if (b == NULL, -1);
    dbg_return_if (b_sz > u_rb_size(rb), -1);

    nop_goto_if (!(to_be_written = U_MIN(u_rb_avail(rb), b_sz)), end);

    memcpy(write_addr(rb), b, to_be_written);

    /* When using the malloc'd buffer, we need to take care of data mirroring
     * manually. */
    if (rb->opts & U_RB_OPT_IMPL_MALLOC)
        (void) mirror(rb, b, to_be_written);
    
    write_incr_contiguous(rb, to_be_written);

    /* fall through */
end:
    return to_be_written;
}

static int mirror (u_rb_t *rb, const void *b, size_t to_be_written)
{
    size_t right_sz, left_sz;

    if (rb->wr_off + to_be_written <= rb->sz)   /* Left. */
    {
        memcpy(write_addr(rb) + rb->sz, b, to_be_written);
    } 
    else if (rb->wr_off >= rb->sz)              /* Right. */
    {
        memcpy(write_addr(rb) - rb->sz, b, to_be_written); 
    } 
    else    /* !Right && !Left (i.e. Cross). */
    {
        left_sz = rb->sz - rb->wr_off;
        right_sz = to_be_written - left_sz;

        /* When the write has crossed the middle, we need to scatter the
         * left and right side of the buffer at the end of the second
         * half and at the beginning of the first half respectively. */
        memcpy(write_addr(rb) + rb->sz, b, left_sz);
        memcpy(rb->base, (const char *) b + left_sz, right_sz);
    }

    return 0;
}

static ssize_t write_wrapped (u_rb_t *rb, const void *b, size_t b_sz)
{
    size_t to_be_written, rspace, rem;

    dbg_return_if (rb == NULL, -1);
    dbg_return_if (b == NULL, -1);
    dbg_return_if (b_sz > u_rb_size(rb), -1);

    nop_goto_if (!(to_be_written = U_MIN(u_rb_avail(rb), b_sz)), end);

    /* If rb has wrapped-around, or the requested amount of data
     * don't need to be fragmented between tail and head, append 
     * it at the write offset. */
    if (is_wrapped(rb) || (rspace = (rb->sz - rb->wr_off)) >= to_be_written)
    {
        memcpy(write_addr(rb), b, to_be_written);
    }
    else
    {
        rem = to_be_written - rspace;
        /* Handle head-tail fragmentation: the available buffer space on the
         * right side is not enough to hold the supplied data. */
        memcpy(write_addr(rb), b, rspace);
        memcpy(rb->base, (const char *) b + rspace, rem);
    }

    write_incr_wrapped(rb, to_be_written);

    /* fall through */
end:
    return to_be_written;
}

static ssize_t read_wrapped (u_rb_t *rb, void *b, size_t b_sz)
{
    size_t to_be_read, rspace, rem;

    dbg_return_if (b == NULL, -1);
    dbg_return_if (rb == NULL, -1);
    dbg_return_if (b_sz > u_rb_size(rb), -1);

    /* if there is nothing ready to be read go out immediately */
    nop_goto_if (!(to_be_read = U_MIN(u_rb_ready(rb), b_sz)), end);

    if (!is_wrapped(rb) || (rspace = (rb->sz - rb->rd_off)) <= to_be_read)
    {
        memcpy(b, read_addr(rb), to_be_read);
        read_incr_wrapped(rb, to_be_read);
    }
    else
    {
        rem = to_be_read - rspace;
        memcpy(b, read_addr(rb), rspace); 
        memcpy((char *) b + rspace, rb->base, rem);
        rb->rd_off = rem;
    }

    /* fall through */
end:
    return to_be_read;
}

static ssize_t read_contiguous (u_rb_t *rb, void *b, size_t b_sz)
{
    size_t to_be_read;

    dbg_return_if (b == NULL, -1);
    dbg_return_if (rb == NULL, -1);
    dbg_return_if (b_sz > u_rb_size(rb), -1);

    /* if there is nothing ready to be read go out immediately */
    nop_goto_if (!(to_be_read = U_MIN(u_rb_ready(rb), b_sz)), end);

    memcpy(b, read_addr(rb), to_be_read);
    read_incr_contiguous(rb, to_be_read);

    /* fall through */
end:
    return to_be_read;
}

#if defined(U_RB_CAN_MMAP)

static int create_mmap (size_t hint_sz, int opts, u_rb_t **prb)
{
    int fd = -1;
    u_rb_t *rb = NULL;
    char path[] = "/tmp/rb-XXXXXX";

    dbg_err_sif ((rb = u_zalloc(sizeof(u_rb_t))) == NULL);
    dbg_err_sif ((fd = mkstemp(path)) == -1);
    dbg_err_sif (u_remove(path));
 
    /* round the supplied size to a page multiple (mmap is quite picky
     * about page boundary alignment) */
    rb->sz = round_sz(hint_sz);
    rb->wr_off = rb->rd_off = rb->ready = 0;
    rb->opts = opts;

    /* Set mmap methods.  Always use the contiguous methods, silently 
     * ignoring any contraddictory user request. */
    rb->cb_free = free_mmap;
    rb->cb_read = read_contiguous;
    rb->cb_fast_read = fast_read;
    rb->cb_write = write_contiguous;

    dbg_err_sif (ftruncate(fd, rb->sz) == -1);

    /* mmap 2*rb->sz bytes. this is just a commodity map that will be 
     * discarded by the two following "half" maps.  we use it just to let the 
     * system choose a suitable base address (one that we are sure it's 
     * a multiple of the page size) that we can safely reuse later on when
     * pretending to MAP_FIXED */
    rb->base = mmap(NULL, rb->sz << 1, PROT_NONE, 
            MAP_ANON | MAP_PRIVATE, -1, 0);
    dbg_err_sif (rb->base == MAP_FAILED);

    /* POSIX: "The mapping established by mmap() shall replace any previous 
     * mappings for those whole pages containing any part of the address space 
     * of the process starting at pa and continuing for len bytes."
     * So, next two mappings replace in-toto the first 'rb->base' map which
     * does not need to be explicitly munmap'd */
 
    /* first half of the mmap'd region.  use MAP_SHARED to "twin" the two
     * mmap'd regions: each byte stored at a given offset in the first half
     * will show up at the same offset in the second half and viceversa */
    dbg_err_sif (mmap(rb->base, rb->sz, PROT_READ | PROT_WRITE, 
            MAP_FIXED | MAP_SHARED, fd, 0) != rb->base);
  
    /* second half is first's twin: they are attached to the same file 
     * descriptor 'fd', hence their pairing is handled at the OS level */
    dbg_err_sif (mmap(rb->base + rb->sz, rb->sz, PROT_READ | PROT_WRITE,
            MAP_FIXED | MAP_SHARED, fd, 0) != rb->base + rb->sz);
 
    /* dispose the file descriptor */
    dbg_err_sif (close(fd) == -1);

    *prb = rb;

    return 0;
err:
    u_rb_free(rb);
    U_CLOSE(fd);
    return -1;
}

static void free_mmap (u_rb_t *rb)
{
    nop_return_if (rb == NULL, );

    /* "All pages containing a part of the indicated range are unmapped",
     * hence the following single munmap with a double length should be ok for 
     * both previous (contiguous) mmap's */
    dbg_return_sif (rb->base && (munmap(rb->base, rb->sz << 1) == -1), );
    u_free(rb);

    return;
}

/* round requested size to a multiple of PAGESIZE */
static size_t round_sz (size_t sz)
{
    size_t pg_sz = (size_t) u_vm_page_sz;

    return !sz ? pg_sz : (((sz - 1) / pg_sz) + 1) * pg_sz;
}

#endif  /* U_RB_CAN_MMAP */

static int create_malloc (size_t sz, int opts, u_rb_t **prb)
{
    size_t real_sz;
    u_rb_t *rb = NULL;

    dbg_return_if (sz == 0, -1);
    dbg_return_if (prb == NULL, -1);

    dbg_err_sif ((rb = u_zalloc(sizeof *rb)) == NULL);

    rb->opts = opts;

    /* Initialize counters. */
    rb->wr_off = rb->rd_off = rb->ready = 0;
    rb->sz = real_sz = sz;

    /* Double buffer size in case the user requested contiguous memory. */
    if (opts & U_RB_OPT_USE_CONTIGUOUS_MEM)
        real_sz <<= 1;

    /* Make room for the buffer. */
    dbg_err_sif ((rb->base = u_zalloc(real_sz)) == NULL);

    /* Set implementation specific callbacks. */
    rb->cb_free = free_malloc;

    if (opts & U_RB_OPT_USE_CONTIGUOUS_MEM) 
    {
        rb->cb_fast_read = fast_read;
        rb->cb_write = write_contiguous;
        rb->cb_read = read_contiguous;
    }
    else
    {
        rb->cb_fast_read = NULL;
        rb->cb_write = write_wrapped;
        rb->cb_read = read_wrapped;
    }

    *prb = rb;

    return 0;
err:
    free_malloc(rb);
    return -1;
}

static void free_malloc (u_rb_t *rb)
{
    nop_return_if (rb == NULL, );

    if (rb->base)
        u_free(rb->base);
    u_free(rb);

    return;
}
