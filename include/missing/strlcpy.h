/* 
 * Copyright (c) 2005-2008 by KoanLogic s.r.l. - All rights reserved.  
 */
#ifndef _LIBU_STRCPY_H_
#define _LIBU_STRCPY_H_
#include <u/libu_conf.h>
#include <sys/types.h>

#ifdef HAVE_STRCPY
#include <string.h>
#else

#ifdef __cplusplus
extern "C" {
#endif

size_t strlcpy(char *dst, const char *src, size_t siz);

#ifdef __cplusplus
}
#endif

#endif

#endif
