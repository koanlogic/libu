/* 
 * Copyright (c) 2005, KoanLogic s.r.l. - All rights reserved.  
 */
#ifndef _LIBU_UNLINK_H_
#define _LIBU_UNLINK_H_
#include "conf.h"

#ifdef HAVE_UNLINK
#include <unistd.h>
#else
int unlink(const char *pathname);
#endif

#endif
