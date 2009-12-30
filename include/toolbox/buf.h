/* $Id: buf.h,v 1.3 2009/12/30 15:19:48 tho Exp $ */

#ifndef _U_LIBU_BUF_H_
#define _U_LIBU_BUF_H_

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

struct u_buf_s;
typedef struct u_buf_s u_buf_t;

int u_buf_append(u_buf_t *buf, const void *data, size_t size);
int u_buf_clear(u_buf_t *buf);
int u_buf_detach(u_buf_t *buf);
int u_buf_set(u_buf_t *buf, const void *data, size_t size);
int u_buf_load(u_buf_t *buf, const char *fqn);
int u_buf_free(u_buf_t *buf);
int u_buf_create(u_buf_t **pbuf);
int u_buf_reserve(u_buf_t *buf, size_t size);
void *u_buf_ptr(u_buf_t *buf);
size_t u_buf_len(u_buf_t *buf);
size_t u_buf_size(u_buf_t *buf);
int u_buf_printf(u_buf_t *ubuf, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* !_U_LIBU_BUF_H_ */
