/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _LIBU_SETENV_H_
#define _LIBU_SETENV_H_

#include <u/libu_conf.h>

#ifdef HAVE_SETENV
  #include <stdlib.h>
#else   /* !HAVE_SETENV */

#ifdef __cplusplus
extern "C" {
#endif

int setenv(const char *name, const char *value, int overwrite);

#ifdef __cplusplus
}
#endif

#endif  /* HAVE_SETENV */
#endif  /* !_LIBU_SETENV_H_ */
