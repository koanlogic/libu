/*
 * Copyright (c) 2005, KoanLogic s.r.l. - All rights reserved.  
 */

static const char rcsid[] =
    "$Id: log.c,v 1.2 2005/10/17 18:21:59 tat Exp $";

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <u/log.h>
#include <u/carpal.h>
#include <u/misc.h>
#include <u/os.h>

/* applications that use libu will defined their own "int facility" variable */
extern int facility;

/* log hook. if not-zero use this function to write log messages */
static u_log_hook_t hook = NULL;

/* messages longer then MAX_LINE_LENGTH will be silently discarded */
enum { MAX_LINE_LENGTH  = 2048 };

static inline const char* u_log_label(int lev)
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
        syslog(LOG_WARNING, 
               "[wrn][%d:::] unknown log level: %d", getpid(), lev);
        return "unk";
    }
}


static int u_log(int priority, const char *fmt, ...)
{
    va_list ap;
    char buf[MAX_LINE_LENGTH];

    va_start(ap, fmt); 

    if(hook)
    {
        if(vsnprintf(buf, MAX_LINE_LENGTH, fmt, ap) > MAX_LINE_LENGTH)
        {
            va_end(ap);
            return ~0; /* buffer too small */
        }
        buf[MAX_LINE_LENGTH] = 0; 
        hook(buf, strlen(buf));
    } else 
        vsyslog(priority, fmt, ap);

    va_end(ap);

    return 0;
}

int u_set_log_hook(u_log_hook_t func, u_log_hook_t *old)
{
    dbg_return_if(func == NULL, ~0);

    if(old)
        *old = hook;
    hook = func;

    return 0;
}

int u_log_write_ex(int fac, int lev, int ctx, const char* file, int line, 
    const char *func, const char* fmt, ...)
{
    va_list ap;
    char msg[MAX_LINE_LENGTH];
    int rc;

    /* build the message to send to the log system */
    va_start(ap, fmt); 
    rc = vsnprintf(msg, MAX_LINE_LENGTH, fmt, ap);
    va_end(ap);

    if(rc > MAX_LINE_LENGTH)
        return ~0; /* message too long */

    /* ok, send the msg to the logger */
    if(ctx)
        u_log(fac | lev, "[%s][%d:%s:%d:%s] %s", 
               u_log_label(lev), getpid(), file, line, func, msg);
    else
        u_log(fac | lev, "[%s][%d:::] %s", 
               u_log_label(lev), getpid(), msg);

    return 0;
}
