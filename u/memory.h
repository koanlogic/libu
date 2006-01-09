/*
 * Copyright (c) 2005, 2006 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_ALLOC_H_
#define _U_ALLOC_H_
#include "libu_conf.h"
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void *u_malloc(size_t sz);
void *u_zalloc(size_t sz);
void *u_calloc(size_t cnt, size_t sz);
void *u_realloc(void *ptr, size_t sz);
void u_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* !_U_ALLOC_H_ */

