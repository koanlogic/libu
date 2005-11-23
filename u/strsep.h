/* 
 * Copyright (c) 2005, KoanLogic s.r.l. - All rights reserved.  
 */
#ifndef _LIBU_STRSEP_H_
#define _LIBU_STRSEP_H_
#include "libu_conf.h"

#ifdef HAVE_STRSEP
#include <string.h>
#else
char * strsep(char **, const char *);
#endif

#endif
