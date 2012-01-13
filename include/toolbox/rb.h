/*
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_RB_H_
#define _U_RB_H_

#include <u/libu_conf.h>

/* Test if we have all the pieces needed to provide the mmap-based 
 * implementation. */
#if defined(HAVE_MMAP) && defined(HAVE_MAP_FIXED) && !defined(RB_INHIBIT_MMAP)
  #define U_RB_CAN_MMAP
#endif

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* forward decl */
struct u_rb_s;

/**
 *  \addtogroup rb
 *  \{
 */ 

/** \brief  The ring buffer type. */
typedef struct u_rb_s u_rb_t;

/** \brief  Options to tweak the ring buffer implementation */
typedef enum {
    U_RB_OPT_NONE               = 0x00,
    /**< use default (depending on platform) creation method */

    U_RB_OPT_USE_CONTIGUOUS_MEM = 0x01,
    /**< force implementation to use a contiguous memory buffer to store
     *   ringbuffer data. This option enables the ::u_rb_fast_read interface. */

    U_RB_OPT_IMPL_MALLOC        = 0x02,
    /**< Force use of malloc(3) based implementation.  The default is to
     *   use the mmap(2) implementation on platforms supporting it. */
} u_rb_opts_t;

int u_rb_create (size_t hint_sz, int opts, u_rb_t **prb);
int u_rb_clear (u_rb_t *rb);
void u_rb_free (u_rb_t *rb);
size_t u_rb_size (u_rb_t *rb);

ssize_t u_rb_read (u_rb_t *rb, void *b, size_t b_sz);
void *u_rb_fast_read (u_rb_t *rb, size_t *pb_sz);
ssize_t u_rb_write (u_rb_t *rb, const void *b, size_t b_sz);
size_t u_rb_ready (u_rb_t *rb);
size_t u_rb_avail (u_rb_t *rb);

/**
 *  \}
 */ 

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* !_U_RB_H_ */
