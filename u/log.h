/* 
 * Copyright (c) 2005, KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_LOG_H_
#define _U_LOG_H_

#include <stdlib.h>
#include <stdarg.h>

#include <u/logprv.h>
#include <u/logcfg.h>

/**
 *  \defgroup log Logging
 *  \{
 *      \par Logging levels
 * 
 *          \li LOG_ERR
 *          \li LOG_WARNING
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
 *              \li \c ctx:
 *                  if &gt; 0 include the filename, line number 
 *                  and function name in the logged message
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

const char* u_log_label(int lev);
char *u_log_build_message(const char *fmt, va_list ap, int maxsz);

/** \brief log an error message and die 
 *
 * Write an error log message and die.
 *
 * \param ecode     exit code
 * \param facility  facility
 * \param ctx       set to zero if you don't want context, 1 otherwise
 * \param args      printf-style variable length arguments list
 */
#define u_log_err(ecode, facility, ctx, ...) \
    do {                                                    \
        u_log_write(facility, LOG_ERR, ctx, __VA_ARGS__);   \
        exit(ecode);                                        \
    } while(0)

/** \brief log a warning message
 *
 * Write a warning log message.
 *
 * \param facility  facility
 * \param ctx       set to zero if you don't want context, 1 otherwise
 * \param args      printf-style variable length arguments list
 */
#define u_log_warning(facility, ctx, ...) \
    u_log_write(facility, LOG_WARNING, ctx, __VA_ARGS__)

/** \brief log an informational message
 *
 * Write an informational log message.
 *
 * \param facility  facility
 * \param ctx       set to zero if you don't want context, 1 otherwise
 * \param args      printf-style variable length arguments list
 */
#define u_log_info(facility, ctx, ...) \
    u_log_write(facility, LOG_INFO, ctx, __VA_ARGS__)

/** \brief log a debug message
 *
 * Write a debug log message.
 *
 * \param facility  facility
 * \param ctx       set to zero if you don't want context, 1 otherwise
 * \param args      printf-style variable length arguments list
 */
#define u_log_debug(facility, ctx, ...) \
    u_log_write(facility, LOG_DEBUG, ctx, __VA_ARGS__)

/** \brief same as u_log_err but using the \e facility global variable */
#define die(ecode, ...) u_log_err(ecode, facility, 1, __VA_ARGS__)

/** \brief calls die() if \e expr is true */
#define die_if(expr) if(expr) die(EXIT_FAILURE, #expr)

/** \brief same as u_log_warning but using the facility global variable */
#define warning(...) u_log_warning(facility, 1, __VA_ARGS__)

/** \brief same as u_log_info but using the facility global variable */
#define info(...) u_log_info(facility, 0, __VA_ARGS__)

/** \brief same as u_log_debug but using the facility global variable */
#define debug(...) u_log_debug(facility, 1, __VA_ARGS__)

/**
 *  \}
 */

#endif /* !_U_LOG_H_ */
