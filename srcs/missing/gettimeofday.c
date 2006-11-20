/* $Id: gettimeofday.c,v 1.1 2006/11/20 13:38:01 tho Exp $ */

#include <u/libu_conf.h>

#ifndef HAVE_GETTIMEOFDAY
#include <toolbox/carpal.h>
#include <missing/gettimeofday.h>

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
#warning missing gettimeofday,tv.tv_usec will be always set to zero
int gettimeofday(struct timeval *tv, struct timezone *tzp)
{
	if(tzp)
		tzp->tz_minuteswest = tzp->tz_dsttime = 0;

	tv->tv_sec = time(0);
	tv->tv_usec = 0;

	return 0;
}
#endif

#else
#include <sys/time.h>
int gettimeofday(struct timeval *tp, struct timezone *tzp);
#endif  /* !HAVE_GETTIMEOFDAY */
