/* 
 * Copyright (c) 2005-2008 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _LIBU_MKSTEMPS_H_
#define _LIBU_MKSTEMPS_H_

#include <u/libu_conf.h>

#ifdef HAVE_MKSTEMPS
#include <stdlib.h>
#else

#ifdef __cplusplus
extern "C" {
#endif

int mkstemps (char *template, int suffixlen);

#ifdef __cplusplus
}
#endif

#endif  /* HAVE_MKTEMP */

#endif  /* !_LIBU_MKSTEMPS_H_ */
