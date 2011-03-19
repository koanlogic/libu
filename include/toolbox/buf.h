/* $Id: buf.h,v 1.5 2010/01/20 16:04:50 tho Exp $ */

#ifndef _U_LIBU_BUF_H_
#define _U_LIBU_BUF_H_

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* forward decl */
struct u_buf_s;

/**
 *  \addtogroup buf
 *  \{
 */

/** \brief  The basic buffer type.  Internally it hides the data buffer
 *          which can be accessed via ::u_buf_ptr, and related size indicators
 *          (both total ::u_malloc'd buffer size, and the number of actually 
 *          used bytes) which are accessed via ::u_buf_size and ::u_buf_len
 *          respectively. */
typedef struct u_buf_s u_buf_t;

int u_buf_append (u_buf_t *buf, const void *data, size_t size);
int u_buf_clear (u_buf_t *buf);
int u_buf_create (u_buf_t **pbuf);
int u_buf_detach (u_buf_t *buf);
int u_buf_free (u_buf_t *buf);
int u_buf_load (u_buf_t *buf, const char *fqn);
int u_buf_printf (u_buf_t *ubuf, const char *fmt, ...);
int u_buf_reserve (u_buf_t *buf, size_t size);
int u_buf_save (u_buf_t *ubuf, const char *filename);
int u_buf_set (u_buf_t *buf, const void *data, size_t size);
ssize_t u_buf_len (u_buf_t *buf);
ssize_t u_buf_size (u_buf_t *ubuf);
void *u_buf_ptr (u_buf_t *buf);
int u_buf_shrink(u_buf_t *ubuf, size_t newlen);

/**
 *  \}
 */

#ifdef __cplusplus
}
#endif

#endif /* !_U_LIBU_BUF_H_ */
