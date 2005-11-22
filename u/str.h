/* 
 * Copyright (c) 2005, KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_STRING_H_
#define _U_STRING_H_
#include "libu_conf.h"

#include <sys/types.h>

enum { BLOCK_SIZE = 64 };

struct u_string_s;
typedef struct u_string_s u_string_t;

#define STRING_NULL { NULL, 0, 0, 0 };

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

int u_string_url_encode(u_string_t *s);
int u_string_url_decode(u_string_t *s);

int u_string_html_encode(u_string_t *s);
int u_string_html_decode(u_string_t *s);

int u_string_sql_encode(u_string_t *s);
int u_string_sql_decode(u_string_t *s);

#endif /* !_U_STRING_H_ */
