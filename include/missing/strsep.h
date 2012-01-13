/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */
#ifndef _LIBU_STRSEP_H_
#define _LIBU_STRSEP_H_

#include <u/libu_conf.h>

#ifdef HAVE_STRSEP
#include <string.h>
#else   /* !HAVE_STRSEP */

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

char *strsep(char **, const char *);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* HAVE_STRSEP */

#endif  /* _LIBU_STRSEP_H_ */
