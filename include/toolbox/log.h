/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_LOG_H_
#define _U_LOG_H_

#include <u/libu_conf.h>

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include <u/missing.h>
#include <u/toolbox/logprv.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  \defgroup log Logging
 *  \{
 *      \par Logging levels
 *
 *          All standard syslog(3) levels:
 * 
 *          \li LOG_EMERG
 *          \li LOG_ALERT
 *          \li LOG_CRIT
 *          \li LOG_ERR
 *          \li LOG_WARNING
 *          \li LOG_NOTICE
 *          \li LOG_INFO
 *          \li LOG_DEBUG
 *
 *          \par NOTE
 *              All functions that not contain the [facility] parameter 
 *              reference a variable named "facility" that \b must be 
 *              defined somewhere and must be in scope.
 *
 *          \par Common parameters
 *
 *              \li \c facility:
 *                  logging facility (see syslog(3))
 *              \li \c flags:
 *                  OR-ed LOG_WRITE_FLAG_* flags
 *              \li \c args:
 *                  printf-style variable argument lists
 *              \li \c expr:
 *                  C expression to evaluate
 *              \li \c err: 
 *                  function return code 
 *              \li \c gt:  
 *                  goto label
 *              \li \c ecode:
 *                  process exit code
 */

/* messages longer then U_MAX_LOG_LENGTH will be silently discarded */
enum { U_MAX_LOG_LENGTH  = 1024 };

/** \brief per-process facility variable.
 *
 * all processes that use the libu must define a "facility" variable somewhere
 * to satisfy this external linkage reference.
 * 
 * Such variable will be used as the syslog(3) facility argument.
 *
 */
extern int facility;

/** \brief log hook typedef */
typedef int (*u_log_hook_t)(void *arg, int level, const char *str); 

/** \brief thread lock callback typedef */
typedef int (*u_log_lock_t)(void *arg);

/** \brief thread unlock callback typedef */
typedef int (*u_log_unlock_t)(void *arg);

/** \brief set the lock function callback
 *
 * Set the lock function used by the log subsystem to work properly in 
 * multi-thread environments (you must also set the unlock function).
 *
 * The lock primitive must allow recursive locking i.e. the thread that owns
 * the lock can call the lock function more times without blocking (it must
 * call the unlock function the same number of times).
 *
 * \param f         function that will be called to get the lock
 * \param arg       an opaque argument that will be passed to the lock function
 *
 * \return 
 *   0 on success, not zero on error
 *
 */
int u_log_set_lock(u_log_lock_t f, void *arg);

/** \brief set the unlock function callback 
 *
 * Set the unlock function used by the log subsystem to work properly in 
 * multi-thread environments (you must also set the lock function);
 *
 * \param f         function that will be called to release the lock
 * \param arg       an opaque argument that will be passed to the lock function
 *
 * \return 
 *   0 on success, not zero on error
 *
 */
int u_log_set_unlock(u_log_unlock_t f, void *arg);

/** \brief set a log hook to redirect log messages
 *
 * Force the log subsystem to use user-provided function to write log messages.
 *
 * The provided function will be called for each dbg_, warn_ or info_ calls.
 *
 * \param hook      function that will be called to write log messages 
 *                  set this param to NULL to set the default syslog-logging
 * \param arg       an opaque argument that will be passed to the hook function
 * \param old       [out] will get the previously set hook or NULL if no hook
 *                  has been set
 * \param parg      [out] will get the previously set hook argument
 *
 * \return 
 *   0 on success, not zero on error
 *
 */
int u_log_set_hook(u_log_hook_t hook, void *arg, u_log_hook_t *old, void**parg);

/** \brief log an error message and die 
 *
 * Write an error log message and die.
 *
 * \param ecode     exit code
 * \param facility  facility
 * \param flags     OR-ed LOG_WRITE_FLAG_* flags
 * \param err       if set append strerror(err) to the message
 * \param ...       printf-style variable length arguments list
 */
#define u_log_die(ecode, facility, flags, err, ...)               \
    do {                                                        \
        u_log_write(facility, LOG_CRIT, flags, err, __VA_ARGS__); \
        exit(ecode);                                            \
    } while(0)

/** \brief log an emerg message
 *
 * Write an emerg log message.
 *
 * \param facility  facility
 * \param flags     OR-ed LOG_WRITE_FLAG_* flags
 * \param err       if set append strerror(err) to the message
 * \param ...       printf-style variable length arguments list
 */
#define u_log_emerg(facility, flags, err, ...) \
    u_log_write(facility, LOG_EMERG, flags, err, __VA_ARGS__)

/** \brief log an alert message
 *
 * Write an alert log message.
 *
 * \param facility  facility
 * \param flags     OR-ed LOG_WRITE_FLAG_* flags
 * \param err       if set append strerror(err) to the message
 * \param ...       printf-style variable length arguments list
 */
#define u_log_alert(facility, flags, err, ...) \
    u_log_write(facility, LOG_ALERT, flags, err, __VA_ARGS__)

/** \brief log a critical message
 *
 * Write a critical log message.
 *
 * \param facility  facility
 * \param flags     OR-ed LOG_WRITE_FLAG_* flags
 * \param err       if set append strerror(err) to the message
 * \param ...       printf-style variable length arguments list
 */
#define u_log_critical(facility, flags, err, ...) \
    u_log_write(facility, LOG_CRIT, flags, err, __VA_ARGS__)

/** \brief log an error message
 *
 * Write an error log message.
 *
 * \param facility  facility
 * \param flags     OR-ed LOG_WRITE_FLAG_* flags
 * \param err       if set append strerror(err) to the message
 * \param ...       printf-style variable length arguments list
 */
#define u_log_error(facility, flags, err, ...) \
    u_log_write(facility, LOG_ERR, flags, err, __VA_ARGS__)

/** \brief log a warning message
 *
 * Write a warning log message.
 *
 * \param facility  facility
 * \param flags     OR-ed LOG_WRITE_FLAG_* flags
 * \param err       if set append strerror(err) to the message
 * \param ...       printf-style variable length arguments list
 */
#define u_log_warning(facility, flags, err, ...) \
    u_log_write(facility, LOG_WARNING, flags, err, __VA_ARGS__)

/** \brief log a notice message
 *
 * Write a notice log message.
 *
 * \param facility  facility
 * \param flags     OR-ed LOG_WRITE_FLAG_* flags
 * \param err       if set append strerror(err) to the message
 * \param ...       printf-style variable length arguments list
 */
#define u_log_notice(facility, flags, err, ...) \
    u_log_write(facility, LOG_NOTICE, flags, err, __VA_ARGS__)

/** \brief log an informational message
 *
 * Write an informational log message.
 *
 * \param facility  facility
 * \param flags     OR-ed LOG_WRITE_FLAG_* flags
 * \param err       if set append strerror(err) to the message
 * \param ...       printf-style variable length arguments list
 */
#define u_log_info(facility, flags, err, ...) \
    u_log_write(facility, LOG_INFO, flags, err, __VA_ARGS__)

/** \brief log a debug message
 *
 * Write a debug log message.
 *
 * \param facility  facility
 * \param flags     OR-ed LOG_WRITE_FLAG_* flags
 * \param err       if set append strerror(err) to the message
 * \param ...       printf-style variable length arguments list
 */
#define u_log_debug(facility, flags, err, ...) \
    u_log_write(facility, LOG_DEBUG, flags, err, __VA_ARGS__)

/** \brief same as u_log_die but using the \e facility global variable */
#define die(ecode, ...) u_log_die(ecode, facility, 1, 0, __VA_ARGS__)

/** \brief calls die() if \e expr is true */
#define die_if(expr) if(expr) die(EXIT_FAILURE, #expr)

/** \brief same as u_log_emerg but using the facility global variable */
#define emerg_( err, ...) u_log_emerg(facility, 1, err, __VA_ARGS__)

/** \brief same as u_log_alert but using the facility global variable */
#define alert_( err, ...) u_log_alert(facility, 1, err, __VA_ARGS__)

/** \brief same as u_log_critical but using the facility global variable */
#define crit_( err, ...) u_log_critical(facility, 1, err, __VA_ARGS__)

/** \brief same as u_log_error but using the facility global variable */
#define err_( err, ...) u_log_error(facility, 1, err, __VA_ARGS__)

/** \brief same as u_log_warning but using the facility global variable */
#define warn_( err, ...) u_log_warning(facility, 1, err, __VA_ARGS__)

/** \brief same as u_log_info but using the facility global variable */
#define notice_( err, ...) u_log_notice(facility, 1, err, __VA_ARGS__)

/** \brief same as u_log_info but using the facility global variable */
#define info_( err, ...) u_log_info(facility, 0, err, __VA_ARGS__)

/** \brief same as u_log_debug but using the facility global variable */
#define dbg_( err, ...) u_log_debug(facility, 1, err, __VA_ARGS__)

/** \brief write a log message to stderr */
#define con_( err, ...) u_console_write( err, __VA_ARGS__)

/**
 *  \brief  Return, in the given buffer, a string describing the error code
 *
 *  Return in 'msg' a string describing the error code. Works equally with 
 *  POSIX-style C libs and with glibc (that use a different prototype for 
 *  strerror_r).
 *
 *  If strerror_r doesn't exist in the system strerror() is used instead.
 *
 *  \param  err     the error code
 *  \param  buf     the buffer that will get the error message
 *  \param  size    size of buf
 *
 *  \return \c 0 on success, \c ~0 on error
 */ 
int u_strerror_r(int err, char *buf, size_t size);

/**
 *  \}
 */

#ifdef __cplusplus
}
#endif

#endif /* !_U_LOG_H_ */
