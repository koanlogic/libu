/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */
#ifndef _LIBU_STRLCAT_H_
#define _LIBU_STRLCAT_H_

#include <u/libu_conf.h>
#include <sys/types.h>
#include <string.h>

#ifdef HAVE_STRLCAT
#include <string.h>
#else   /* !HAVE_STRLCAT */

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

size_t strlcat(char *dst, const char *src, size_t siz);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* HAVE_STRLCAT */

#endif  /* !_LIBU_STRLCAT_H_ */
