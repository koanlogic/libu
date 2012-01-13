/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */
#ifndef _LIBU_GETTIMEOFDAY_H_
#define _LIBU_GETTIMEOFDAY_H_
#include <u/libu_conf.h>
#include <time.h>
#include <sys/time.h>

#ifndef HAVE_GETTIMEOFDAY

#ifdef __cplusplus
extern "C" {
#endif

struct timezone
{
    int tz_minuteswest;
    int tz_dsttime;
};

int gettimeofday(struct timeval *tp, struct timezone *tzp);

#ifdef __cplusplus
}
#endif

#endif /* HAVE_GETTIMEOFDAY */

#endif
