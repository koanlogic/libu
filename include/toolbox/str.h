/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
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

struct u_string_s;

/**
 *  \addtogroup string
 *  \{
 */ 

/** \brief  The base string type */
typedef struct u_string_s u_string_t;

/**
 *  \brief  Append at most \p len characters of a NUL-terminated string to a 
 *          string object
 *
 *  Append at most \p len chars from the NUL-terminated string \p buf to the 
 *  given ::u_string_t object \p s
 *
 *  \param  s   an ::u_string_t object
 *  \param  buf the NUL-terminated string that will be appended to \p s
 *  \param  len length of \a buf
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
#define u_string_ncat(s, buf, len)  u_string_append(s, buf, len)

/**
 *  \brief  Append a NUL-terminated string to a string object
 *
 *  Append the NUL-terminated string \p buf to the given ::u_string_t object 
 *  \p s
 *
 *  \param  s   an ::u_string_t object
 *  \param  buf the NUL-terminated string that will be appended to \p s
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */
#define u_string_cat(s, buf)    u_string_append(s, buf, strlen(buf))

const char *u_string_c (u_string_t *s);
int u_string_append (u_string_t *s, const char *buf, size_t len);
int u_string_aprintf (u_string_t *s, const char *fmt, ...);
int u_string_clear (u_string_t *s);
int u_string_copy (u_string_t *dst, u_string_t *src);
int u_string_create (const char *buf, size_t len, u_string_t **ps);
int u_string_do_printf (u_string_t *s, int clear, const char *fmt, ...);
int u_string_free (u_string_t *s);
int u_string_reserve (u_string_t *s, size_t size);
int u_string_set (u_string_t *s, const char *buf, size_t len);
int u_string_set_length (u_string_t *s, size_t len); 
int u_string_sprintf (u_string_t *s, const char *fmt, ...);
int u_string_trim (u_string_t *s);
size_t u_string_len (u_string_t *s);
char *u_string_detach_cstr (u_string_t *s);

/**
 *  \}
 */ 

#ifdef __cplusplus
}
#endif

#endif /* !_U_STRING_H_ */
