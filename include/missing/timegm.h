/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */
#ifndef _LIBU_TIMEGM_H_
#define _LIBU_TIMEGM_H_
#include <u/libu_conf.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HAVE_TIMEGM
time_t timegm(struct tm *tm);
#endif

#ifdef __cplusplus
}
#endif

#endif
