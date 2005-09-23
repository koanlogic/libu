/*
 * Copyright (c) 2005, KoanLogic s.r.l. - All rights reserved.  
 */

static const char rcsid[] =
    "$Id: log.c,v 1.1.1.1 2005/09/23 13:04:38 tho Exp $";

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>

#include <u/log.h>
#include <u/debug.h>
#include <u/logcfg.h>
#include <u/misc.h>

extern int facility;

const char* u_log_label(int lev)
{
    switch(lev)
    {
    case LOG_CRIT:   
        return "crt";
    case LOG_ERR:    
        return "err";
    case LOG_WARNING:
        return "wrn";
    case LOG_NOTICE: 
        return "not"; 
    case LOG_INFO:   
        return "inf"; 
    case LOG_DEBUG:  
        return "dbg"; 
    default: 
        syslog(LF_MISC | LOG_WARNING, 
               "[wrn][%d:::] unknown log level: %d", getpid(), lev);
        return "unk";
    }
}

char *u_log_build_message(const char *fmt, va_list ap, int maxsz)
{
    char *buf;
    int bufsz = 128;
    for(;;)
    {
        buf = (char*)calloc(1, bufsz);
        if(buf == 0)
            return 0;  /* no more memory */
        /* sprintf user message to [buf] */
        if(vsnprintf(buf, bufsz-1, fmt, ap) >= bufsz)
        {   /* buffer too small, resize it and try again */
            if(bufsz >= maxsz)
                break;   /* msg too long, give up. log msg will be truncated */
            bufsz <<= 1; /* doubled */
            U_FREE(buf);   /* free the small buffer */
            continue;
        }
        break;
    }
    return buf; /* must be free()'d outside */
}

int u_log_write_ex(int fac, int lev, int ctx, const char* file, int line, 
    const char *func, const char* fmt, ...)
{
    enum { MAX_BUFSZ = 2048 };
    va_list ap;
    char *buf;

    /* build the message to send to the log system */
    va_start(ap, fmt); /* init variable list arguments */
    if( (buf = u_log_build_message(fmt, ap, MAX_BUFSZ)) == 0)
    {
        syslog(LF_MISC | LOG_WARNING, "[%s][%d:%s:%d:%s] %s", 
               u_log_label(lev), getpid(),  file, line, func, 
               "u_log_write_ex: Not enough memory");
    }
    va_end(ap);
    /* ok, send the msg to the logger */
    if(buf)
    {
        if(ctx)
            syslog(fac | lev, "[%s][%d:%s:%d:%s] %s", 
                   u_log_label(lev), getpid(), file, line, func, buf);
        else
            syslog(fac | lev, "[%s][%d:::] %s", 
                   u_log_label(lev), getpid(), buf);

        /* errors will be printed out on stderr if the prigram has been run
           interactively */
        if(lev < LOG_WARNING && isatty(2))
            fprintf(stderr, "[%s][%d:%s:%d:%s] %s", u_log_label(lev), getpid(), 
                file, line, func, buf);
        U_FREE(buf);
    }
    return 0;
}


#ifdef NDEBUG 
/* used to avoid warnings */
void dbg_nop()
{
    return;
}
#endif
