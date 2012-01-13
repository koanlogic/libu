/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */
#ifndef _LIBU_STRTOK_R_H_
#define _LIBU_STRTOK_R_H_

#include <u/libu_conf.h>

#ifdef HAVE_STRTOK_R
#include <string.h>
#else   /* !HAVE_STRTOK_R */

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

char *strtok_r(char *s, const char *delim, char **last);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* HAVE_STRTOK_R */

#endif  /* !_LIBU_STRTOK_R_H_ */
