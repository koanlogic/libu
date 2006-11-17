/*
 * Copyright (c) 2005, 2006 by KoanLogic s.r.l. - All rights reserved.  
 */

static const char rcsid[] =
    "$Id: log.c,v 1.10 2006/11/17 17:47:47 tat Exp $";

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <u/log.h>
#include <u/carpal.h>
#include <u/misc.h>
#include <u/os.h>
#include <u/syslog.h>

/* applications that use libu will defined their own "int facility" variable */
extern int facility;

/* log hook. if not-zero use this function to write log messages */
static u_log_hook_t hook = NULL;
static void *hook_arg = NULL;

#ifdef OS_WIN
#define err_type DWORD
#define save_errno(var) var = GetLastError();
#define restore_errno(var) SetLastError(var);
#else
#define err_type int
#define save_errno(var) var = errno;
#define restore_errno(var) errno = var;
#endif

static inline const char* u_log_label(int lev)
{
    switch(lev)
    {
    case LOG_DEBUG:  
        return "dbg"; 
    case LOG_INFO:   
        return "inf"; 
    case LOG_NOTICE: 
        return "ntc"; 
    case LOG_WARNING:
        return "wrn";
    case LOG_ERR:    
        return "err";
    case LOG_CRIT:   
        return "crt";
    case LOG_ALERT:   
        return "alr";
    case LOG_EMERG:   
        return "emg";
    default: 
        syslog(LOG_WARNING, 
               "[wrn][%d:::] unknown log level: %d", getpid(), lev);
        return "unk";
    }
}


static int u_log(int fac, int level, const char *fmt, ...)
{
    va_list ap;
    char buf[U_MAX_LOG_LENGTH];

    va_start(ap, fmt); 

    if(hook)
    {
        if(vsnprintf(buf, U_MAX_LOG_LENGTH, fmt, ap) > U_MAX_LOG_LENGTH)
        {
            va_end(ap);
            return ~0; /* buffer too small */
        }
        buf[U_MAX_LOG_LENGTH - 1] = 0; 
        hook(hook_arg, level, buf);
    } else 
        vsyslog(fac | level, fmt, ap);

    va_end(ap);

    return 0;
}

int u_log_set_hook(u_log_hook_t func, void *arg, u_log_hook_t *old, void **parg)
{
    if(old)
        *old = hook;
    if(parg)
        *parg = hook_arg;

    hook = func;
    hook_arg = arg;

    return 0;
}

int u_log_write_ex(int fac, int lev, int ctx, const char* file, int line, 
    const char *func, const char* fmt, ...)
{
    va_list ap;
    char msg[U_MAX_LOG_LENGTH];
    int rc;
    err_type savederr;

    save_errno(savederr);

    /* build the message to send to the log system */
    va_start(ap, fmt); 
    rc = vsnprintf(msg, U_MAX_LOG_LENGTH, fmt, ap);
    va_end(ap);

    if(rc > U_MAX_LOG_LENGTH)
        goto err; /* message too long */

    /* ok, send the msg to the logger */
    if(ctx)
        u_log(fac, lev, "[%s][%d:%s:%d:%s] %s", 
               u_log_label(lev), getpid(), file, line, func, msg);
    else
        u_log(fac, lev, "[%s][%d:::] %s", 
               u_log_label(lev), getpid(), msg);

    restore_errno(savederr);
    return 0;
err:
    restore_errno(savederr);
    return ~0;
}
