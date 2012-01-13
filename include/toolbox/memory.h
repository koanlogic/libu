/*
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_ALLOC_H_
#define _U_ALLOC_H_

#include <u/libu_conf.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void u_memory_set_malloc (void *(*f_malloc)(size_t));
void u_memory_set_calloc (void *(*f_calloc)(size_t, size_t));
void u_memory_set_realloc (void *(*f_realloc)(void *, size_t));
void u_memory_set_free (void (*f_free)(void *));
void *u_malloc (size_t sz);
void *u_calloc (size_t cnt, size_t sz);
void *u_zalloc (size_t sz);
void *u_realloc (void *ptr, size_t sz);
void u_free (void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* !_U_ALLOC_H_ */

