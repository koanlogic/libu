/* 
 * Copyright (c) 2005, KoanLogic s.r.l. - All rights reserved.  
 */
#ifndef _LIBU_STRTOK_R_H_
#define _LIBU_STRTOK_R_H_
#include "libu_conf.h"

#ifdef HAVE_STRTOK_R
#include <string.h>
#else
char * strtok_r(char *s, const char *delim, char **last);
#endif

#endif
