/* 
 * Copyright (c) 2005, KoanLogic s.r.l. - All rights reserved.  
 */
#ifndef _LIBU_GETTIMEOFDAY_H_
#define _LIBU_GETTIMEOFDAY_H_
#include "libu_conf.h"
#include <time.h>
#include <sys/time.h>

#ifndef HAVE_GETTIMEOFDAY

struct timezone
{
    int tz_minuteswest;
    int tz_dsttime;
};

int gettimeofday(struct timeval *tp, struct timezone *tzp);
#endif

#endif
