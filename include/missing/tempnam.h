/* 
 * Copyright (c) 2005-2010 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _LIBU_TEMPNAM_H_
#define _LIBU_TEMPNAM_H_

#include <u/libu_conf.h>

#ifdef HAVE_TEMPNAM
#include <stdio.h>
#else   /* !HAVE_TEMPNAM */

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

char *tempnam (const char *dir, const char *pfx);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* HAVE_TEMPNAM */

#endif  /* !_LIBU_TEMPNAM_H_ */
