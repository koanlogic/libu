/*
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <toolbox/log.h>
#include <toolbox/carpal.h>
#include <toolbox/misc.h>
#include <u/missing.h>
#include <missing/syslog.h>

/* applications that use libu will defined their own "int facility" variable */
extern int facility;

/* log hook. if not-zero use this function to write log messages */
static u_log_hook_t hook = NULL;
static void *hook_arg = NULL;

static u_log_lock_t f_lock = NULL;
static void *f_lock_arg = NULL;

static u_log_unlock_t f_unlock = NULL;
static void *f_unlock_arg = NULL;

enum { STRERR_BUFSZ = 128, ERRMSG_BUFSZ = 256 };

#ifdef OS_WIN
#define err_type DWORD
#define save_errno(var) var = GetLastError();
#define restore_errno(var) SetLastError(var);
#else
#define err_type int
#define save_errno(var) var = errno;
#define restore_errno(var) errno = var;
#endif

int u_log_set_lock(u_log_lock_t f, void *arg)
{
    f_lock = f;
    f_lock_arg = arg;

    return 0;
}

int u_log_set_unlock(u_log_unlock_t f, void *arg)
{
    f_unlock = f;
    f_unlock_arg = arg;

    return 0;
}

static inline void u_log_lock(void)
{
    if(f_lock)
        f_lock(f_lock_arg);
}

static inline void u_log_unlock(void)
{
    if(f_unlock)
        f_unlock(f_unlock_arg);
}

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

    u_log_lock();

    if(hook)
    {
        if(vsnprintf(buf, U_MAX_LOG_LENGTH, fmt, ap) > U_MAX_LOG_LENGTH)
        {
            va_end(ap);
            u_log_unlock();
            return ~0; /* buffer too small */
        }
        buf[U_MAX_LOG_LENGTH - 1] = 0; 
        hook(hook_arg, level, buf);
    } else 
        vsyslog(fac | level, fmt, ap);

    u_log_unlock();

    va_end(ap);

    return 0;
}

int u_log_set_hook(u_log_hook_t func, void *arg, u_log_hook_t *old, void **parg)
{
    u_log_lock();

    if(old)
        *old = hook;
    if(parg)
        *parg = hook_arg;

    hook = func;
    hook_arg = arg;

    u_log_unlock();

    return 0;
}

int u_log_write_ex(int fac, int lev, int flags, int err, const char* file, 
    int line, const char *func, const char* fmt, ...)
{
    va_list ap;
    err_type savederr;
    int rc;
    char msg[U_MAX_LOG_LENGTH], strerr[STRERR_BUFSZ], errmsg[STRERR_BUFSZ];

    save_errno(savederr);

    /* build the message to send to the log system */
    va_start(ap, fmt); 
    rc = vsnprintf(msg, U_MAX_LOG_LENGTH, fmt, ap);
    va_end(ap);

    if(rc >= U_MAX_LOG_LENGTH)
        goto err; /* message too long */

    /* init empty strings */
    errmsg[0] = strerr[0] = 0;

    if(err)
    {
        u_strerror_r(err, strerr, sizeof(strerr));
        snprintf(errmsg, sizeof(errmsg), "[errno: %d, %s]", err, strerr);
        errmsg[sizeof(errmsg) - 1] = 0; /* paranoid set */
    } 

    /* ok, send the msg to the logger */
    if(flags & LOG_WRITE_FLAG_CTX)
        u_log(fac, lev, "[%s][%d:%s:%d:%s] %s %s", 
               u_log_label(lev), getpid(), file, line, func, msg, errmsg);
    else
        u_log(fac, lev, "[%s][%d:::] %s %s", 
               u_log_label(lev), getpid(), msg, errmsg);

    restore_errno(savederr);
    return 0;
err:
    restore_errno(savederr);
    return ~0;
}

int u_console_write_ex(int err, const char* file, int line, 
    const char *func, const char* fmt, ...)
{
    err_type savederr;
    va_list ap;
    int rc;
    char strerr[STRERR_BUFSZ], errmsg[STRERR_BUFSZ];

    /* when writing to console the following parameters are not used */
    file = NULL, line = 0, func = NULL;

    save_errno(savederr);

    /* build the message to send to the log system */
    va_start(ap, fmt); 

    /* write the message to the standard error */
    rc = vfprintf(stderr, fmt, ap);

    va_end(ap);

    if(rc < 0)
        goto err;

    /* init empty strings */
    errmsg[0] = strerr[0] = 0;

    if(err)
    {
        u_strerror_r(err, strerr, sizeof(strerr));
        snprintf(errmsg, sizeof(errmsg), "[errno: %d, %s]", err, strerr);
        errmsg[sizeof(errmsg) - 1] = 0; /* paranoid set */
        fprintf(stderr, " %s\n", errmsg);
    } else
        fprintf(stderr, "\n");

    restore_errno(savederr);
    return 0;
err:
    restore_errno(savederr);
    return ~0;
}

int u_strerror_r(int en, char *msg, size_t sz)
{

#ifdef HAVE_STRERROR_R
    enum { BUFSZ = 256 };
    char buf[BUFSZ] = { 0 };

    intptr_t rc;

    /* assume POSIX prototype */
    rc = (intptr_t)strerror_r(en, buf, BUFSZ);

    if(rc == 0)
    {    /* posix version, success */
        (void) strlcpy(msg, buf, sz);
    } else if(rc == -1 || (rc > 0 && rc < 1024)) {
         /* posix version, failed (returns -1 or an error number) */
         goto err;
    } else {
        /* glibc char*-returning version, always succeeds */
        (void) strlcpy(msg, (char*)rc, sz);
    }
#else
    /* there's not strerror_r, use strerror() instead */
    char *p;

    dbg_err_if((p = strerror(en)) == NULL);

    (void) strlcpy(msg, p, sz);
#endif

    return 0;
err:
    return ~0;
}

