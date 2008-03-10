/* 
 * Copyright (c) 2005-2008 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_LIBU_STRING_H_
#define _U_LIBU_STRING_H_

#include <u/libu_conf.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { BLOCK_SIZE = 64 };

struct u_string_s;
typedef struct u_string_s u_string_t;

#define STRING_NULL { NULL, 0, 0, 0 };

#define u_string_sprintf( s, ... ) \
    u_string_do_printf(s, 1, __VA_ARGS__ )
#define u_string_aprintf( s, ... ) \
    u_string_do_printf(s, 0, __VA_ARGS__ )
#define u_string_cat( s, buf) \
    u_string_append(s, buf, strlen(buf))
#define u_string_ncat( s, buf, len) \
    u_string_append(s, buf, len)

int u_string_create(const char *buf, size_t len, u_string_t **ps);
int u_string_append(u_string_t *s, const char *buf, size_t len);
int u_string_set(u_string_t *s, const char *buf, size_t len);
int u_string_clear(u_string_t *s);
int u_string_free(u_string_t *s);
const char *u_string_c(u_string_t *s);
size_t u_string_len(u_string_t *s);
int u_string_copy(u_string_t *dst, u_string_t *src);
int u_string_set_length(u_string_t *s, size_t len); 
int u_string_trim(u_string_t *s);
int u_string_reserve(u_string_t *s, size_t size);
int u_string_do_printf(u_string_t *s, int clear, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* !_U_STRING_H_ */
