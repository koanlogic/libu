/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */
#ifndef _LIBU_STRLCPY_H_
#define _LIBU_STRLCPY_H_

#include <u/libu_conf.h>
#include <sys/types.h>

#ifdef HAVE_STRLCPY
#include <string.h>
#else   /* !HAVE_STRLCPY */

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

size_t strlcpy(char *dst, const char *src, size_t siz);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* HAVE_STRLCPY */

#endif  /* !_LIBU_STRLCPY_H_ */
