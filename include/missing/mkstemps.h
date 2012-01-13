/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _LIBU_MKSTEMPS_H_
#define _LIBU_MKSTEMPS_H_

#include <u/libu_conf.h>

#ifdef HAVE_MKSTEMPS
#include <stdlib.h>
#else   /* !HAVE_MKSTEMPS */

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

int mkstemps (char *template, int suffixlen);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* HAVE_MKTEMP */

#endif  /* !_LIBU_MKSTEMPS_H_ */
