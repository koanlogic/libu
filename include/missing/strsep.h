/* 
 * Copyright (c) 2005-2010 by KoanLogic s.r.l. - All rights reserved.  
 */
#ifndef _LIBU_STRSEP_H_
#define _LIBU_STRSEP_H_
#include <u/libu_conf.h>

#ifdef HAVE_STRSEP
#include <string.h>
#else

#ifdef __cplusplus
extern "C" {
#endif

char * strsep(char **, const char *);

#ifdef __cplusplus
}
#endif

#endif

#endif
