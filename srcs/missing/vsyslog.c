#include <u/libu_conf.h>
#include <u/missing.h>

/*
 * vsyslog(3) drop-in for Windows and Minix
 */ 

#if !defined(HAVE_VSYSLOG)
#if defined(OS_WIN)

#include <windows.h>
#include <io.h>
#include <sys/locking.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

void vsyslog(int priority, const char *fmt, va_list ap)
{
#define LIBU_WIN_LOGFILE "libu.log"
    enum { BUFSZ = 1024 };
    static FILE *df = NULL, *lock = NULL;
    char buf[BUFSZ];
    int i;

    /* first time open the log file and the lock file */
    if(df == NULL)
    {
        df = fopen(LIBU_WIN_LOGFILE, "a+");
        lock = fopen(LIBU_WIN_LOGFILE ".lock", "a+");
        if(df == NULL || lock == NULL)
            exit(1);
    }

    vsnprintf(buf, BUFSZ, fmt, ap);

    /* get the lock (i.e. lock the first byte of the lock file) */
    for(i = 0; 
        _locking(fileno(lock), _LK_NBLCK, 1) == EACCES && i < 10; ++i)
        Sleep(100);

    if(i < 10)
    {   /* we have the lock, write the log msg */
        fprintf(df, "%s\n", buf);
        fflush(df);
        /* unlock the file */
        _locking(fileno(lock), _LK_UNLCK, 1);
    } else {
        /* file is still locked after 10 attempts, give up */
        ;
    }

    return;
}

#else   /* !OS_WIN */

#include <stdarg.h>
#include <stdio.h>

/* Try with this as last resort (MINIX and QNX for SH target will be glad). */
void vsyslog(int priority, const char *fmt, va_list args)
{
    char buf[1024];

    (void) vsnprintf(buf, sizeof buf, fmt, args);
    syslog(priority, "%s", buf);

    return;
}

#endif  /* OS_WIN */

#else   /* HAVE_VSYSLOG */

#include <syslog.h>
#include <stdarg.h>
void vsyslog(int priority, const char *fmt, va_list args);

#endif  /* !HAVE_VSYSLOG */
