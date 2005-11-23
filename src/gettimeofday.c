#include "libu_conf.h"
#ifndef HAVE_GETTIMEOFDAY
#include <u/carpal.h>
#include <u/gettimeofday.h>

#ifdef OS_WIN
#include <time.h>
#include <sys/timeb.h>
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    struct _timeb tb;

    dbg_return_if(tv == NULL, -1);

    /* get current time */
    _ftime(&tb);

    /* set the timeval struct */
    tv->tv_sec = tb.time;
    tv->tv_usec = 1000 * tb.millitm;

    if(tz == NULL)
        return 0;

    /* set the tiemzone struct */
    tz->tz_minuteswest = tb.timezone;
    tz->tz_dsttime = tb.dstflag;

    return 0;
}
#else
#error define gettimeofday() for this platform
#endif

#else
#include <sys/time.h>
int gettimeofday(struct timeval *tp, struct timezone *tzp);
#endif 
