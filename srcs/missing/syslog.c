#include <u/libu_conf.h>
#include <u/missing.h>

/*
 * syslog(3) drop-in for Windows 
 */ 

#if !defined(HAVE_SYSLOG)
#if defined(OS_WIN)

#include <windows.h>
#include <io.h>
#include <sys/locking.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

void syslog(int priority, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt); /* init variable list arguments */

    vsyslog(priority, fmt, ap);

    va_end(ap);
}

#else   /* !OS_WIN */
    #error "We don't have a syslog(3) implementation for this platform"
#endif  /* OS_WIN */

#else /* HAVE_SYSLOG */

#include <syslog.h>
void syslog(int priority, const char *fmt, ...);

#endif  /* !HAVE_SYSLOG */
