/* 
 * Copyright (c) 2005, 2006 by KoanLogic s.r.l. - All rights reserved.  
 */
#ifndef _LIBU_STRCAT_H_
#define _LIBU_STRCAT_H_
#include <u/libu_conf.h>
#include <sys/types.h>

#ifdef HAVE_STRCAT
#include <string.h>
#else

#ifdef __cplusplus
extern "C" {
#endif

size_t strlcat(char *dst, const char *src, size_t siz);

#ifdef __cplusplus
}
#endif

#endif

#endif
