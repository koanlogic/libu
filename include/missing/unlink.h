/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */
#ifndef _LIBU_UNLINK_H_
#define _LIBU_UNLINK_H_
#include <u/libu_conf.h>

#ifdef HAVE_UNLINK
#include <unistd.h>
#else

#ifdef __cplusplus
extern "C" {
#endif

int unlink(const char *pathname);

#ifdef __cplusplus
}
#endif

#endif

#endif
