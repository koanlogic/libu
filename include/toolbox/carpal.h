/* 
 * Copyright (c) 2005-2007 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_CARPAL_H_
#define _U_CARPAL_H_

#include <u/libu_conf.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>

#include <u/missing/syslog.h>
#include <u/toolbox/log.h>

#ifdef __cplusplus
extern "C" {
#endif

#define msg(label, err, ...) label( err, __VA_ARGS__ )
#define msg_noargs(label, err, literal) label( err, "%s", literal)

/** \brief log a message and goto "err" label
 *
 *   log a message of type \e label if \e expr not zero.
 *
 */
#define msg_err(label, ...) \
    do { msg(label, 0, __VA_ARGS__); goto err; } while(0)

/** \brief log a message if \e expr not zero.
 *
 *   log a message of type \e label if \e expr is not zero 
 *
 *   \e expr text statement will be written to the log file.
 *
 *   For ex.:
 *       warn_if(check(abc) < 0);
 */
#define msg_if(label, expr) do { if( expr ) \
    msg_noargs(label, 0, #expr); } while(0)

/** \brief log the given message if \e expr not zero.
 *
 *   log the given message of type \e label if \e expr is not zero 
 *
 *   For ex.:
 *       warn_ifm(check(abc) < 0, "check failed on file %s", filename);
 */
#define msg_ifm(label, expr, ...) \
    do { if(expr) { msg(label, 0, __VA_ARGS__); } } while(0);

/** \brief log a message if \e expr not zero and enter the if-block
 *
 *   log a message of type \e label if \e expr is not zero and enter the
 *   following if-block.
 *
 *   \e expr text statement will be written to the log file.
 *
 *   A C-style dbg_ code block should follow. For ex.:
 *       dbg_ifb(i == 0)
 *       {
 *           do_something();
 *           do_something();
 *       }
 */
#define msg_ifb(label, expr) \
    if( (expr) && (msg_noargs(label, 0, #expr) ? 1 : 1) ) 

/** \brief log a message if \e expr not zero and return \e err.
 *
 *   log a message of type \e label if \e expr is not zero and return
 *   \e err to the caller.
 *
 *   \e expr text statement will be written to the log file.
 *
 *   Example:
 *      dbg_return_if(param == NULL, FUNC_ERROR);
 */
#define msg_return_if(label, expr, err) msg_ifb(label, expr) { return err; }

/** \brief log the given message if \e expr not zero and return \e err.
 *
 *   log the given message of type \e label if \e expr is not zero and return
 *   \e err to the caller.
 *
 *   Example:
 *    dbg_return_ifm(param == NULL, FUNC_ERROR, "param %d must be not NULL", i);
 */
#define msg_return_ifm(label, expr, err, ...) \
    if(expr) { msg(label, 0, __VA_ARGS__); return err; }

/** \brief log the given message and strerror(errno) if \e expr not zero and return \e err.
 *
 *   log the given message of type \e label and strerror(errno) if \e expr is 
 *   not zero and return \e err to the caller.
 *
 *   Example:
 *    dbg_return_sifm(stat(file, &st) < 0, -1, "file %s access error", file);
 */
#define msg_return_sifm(label, expr, err, ...) \
    if(expr) { msg(label, errno, __VA_ARGS__); return err; }

/** \brief log a message if \e expr not zero and return \e err. (Log the strerror also)
 *
 *   log a message of type \e label if \e expr is not zero and return
 *   \e err to the caller. Log also the strerror(errno).
 *
 *   \e expr text statement will be written to the log file.
 */
#define msg_return_sif(label, expr, err) \
    do { if(expr) { msg_noargs(label, errno, #expr); return err; } } while(0)

/** \brief log a message if \e expr not zero and goto \e gt.
 *
 *   log a message of type \e label if \e expr is not zero and goto
 *   to the label \e gt (that must be in-scope).
 *
 *   \e expr text statement will be written to the log file.
 */
#define msg_goto_if(label, expr, gt) msg_ifb(label, expr) goto gt

/** \brief log a message if \e expr not zero and goto to the label "err"
 *
 *   log a message of type \e label if \e expr is not zero and goto
 *   to the label "err" (that must be defined).
 *
 *   \e expr text statement will be written to the log file.
 */
#define msg_err_if(label, expr) do { msg_ifb(label, expr) { goto err;} } while(0)

/** \brief log the given message if \e expr not zero and goto to the label "err"
 *
 *   log a message of type \e label if \e expr is not zero and goto
 *   to the label "err" (that must be defined). also logs arguments provided
 *   by the caller.
 */
#define msg_err_ifm(label, expr, ...) \
    do { if( (expr) ) { msg(label, 0, __VA_ARGS__); goto err; } } while(0)

/** \brief log a message and strerror(errno) if \e expr not zero; \
 *      then goto to the label "err"
 *
 *   log a message of type \e label if \e expr is not zero and goto
 *   to the label "err" (that must be defined). also logs strerror(errno).
 */
#define msg_err_sif(label, expr) \
    do { if(expr) { msg_noargs(label, errno, #expr); goto err; } } while(0)

/** \brief log the given message and strerror(errno) if \e expr not zero; \
 *      then goto to the label "err"
 *
 *   log the given  message of type \e label if \e expr is not zero, log
 *   strerror(errno) and goto to the label "err" (that must be defined). 
 *   also logs strerror(errno).
 */
#define msg_err_sifm(label, expr, ...) \
    do { if((expr)) { msg(label, errno, __VA_ARGS__);  goto err; } } while(0)


/** \brief write a debug message containing the message returned by strerror(errno) */
#ifdef OS_WIN
#define msg_strerror(label, en)                                     \
    do {                                                            \
        LPVOID lpMsgBuf = NULL;  DWORD dw = GetLastError();         \
        if(FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |          \
            FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw,                   \
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),              \
            (LPTSTR) &lpMsgBuf, 0, NULL ) && lpMsgBuf)              \
        {                                                           \
            msg(label, 0, "%s", lpMsgBuf);                          \
            LocalFree(lpMsgBuf);                                    \
        }                                                           \
    } while(0)
#else
/** \brief write a debug message containing the message returned by strerror(errno) */
#define msg_strerror(label, en)                                     \
    do {                                                            \
        enum { _DBG_BUFSZ = 256 };                                  \
        char _eb[_DBG_BUFSZ] = { 0 };                               \
        if(u_strerror_r(en, _eb, _DBG_BUFSZ)) {                     \
            msg(label, 0, "strerror_r(%d, ...) failed", en);        \
        } else {                                                    \
            msg(label, 0, "errno: %d (%s)", errno, _eb);            \
        }                                                           \
    } while(0)  

#endif /* ! def OS_WIN */

/* nop_ macros */
#define nop_return_if(expr, err)       do { if(expr) return err; } while(0)
#define nop_err_if(expr)               do { if(expr) goto err;   } while(0)
#define nop_goto_if(expr, gt)          do { if(expr) goto gt;    } while(0)

/* con_ macros */
#define con(...)                        msg(con_, 0, __VA_ARGS__)
#define con_err(...)                    msg_err(con_, __VA_ARGS__)
#define con_ifb(expr)                   msg_ifb(con_, expr)
#define con_if(expr)                    msg_if(con_, expr) 
#define con_ifm(expr, ...)              msg_ifm(con_, expr, __VA_ARGS__) 

#define con_return_if(expr, err)        msg_return_if(con_, expr, err)
#define con_return_sif(expr, err)       msg_return_sif(con_, expr, err)
#define con_return_ifm(expr, err, ...)  \
    msg_return_ifm(con_, expr, err, __VA_ARGS__)
#define con_return_sifm(expr, err, ...) \
    msg_return_sifm(con_, expr, err, __VA_ARGS__)

#define con_err_if(expr)                msg_err_if(con_, expr)
#define con_err_sif(expr)               msg_err_sif(con_, expr)
#define con_err_ifm(expr, ...)          msg_err_ifm(con_, expr, __VA_ARGS__)
#define con_err_sifm(expr, ...)         msg_err_sifm(con_, expr, __VA_ARGS__)

#define con_goto_if(expr, gt)           msg_goto_if(con_, expr, gt)
#define con_strerror(err)               msg_strerror(con_, err)

/* emerg_ macros */
#define emerg(...)                      msg(emerg_, 0, __VA_ARGS__)
#define emerg_err(...)                  msg_err(emerg_, __VA_ARGS__)
#define emerg_ifb(expr)                 msg_ifb(emerg_, expr)
#define emerg_if(expr)                  msg_if(emerg_, expr) 
#define emerg_ifm(expr, ...)            msg_ifm(emerg_, expr, __VA_ARGS__) 

#define emerg_return_if(expr, err)      msg_return_if(emerg_, expr, err)
#define emerg_return_sif(expr, err)     msg_return_sif(emerg_, expr, err)
#define emerg_return_ifm(expr, err, ...)    \
    msg_return_ifm(emerg_, expr, err, __VA_ARGS__)
#define emerg_return_sifm(expr, err, ...)   \
    msg_return_sifm(emerg_, expr, err, __VA_ARGS__)

#define emerg_err_if(expr)              msg_err_if(emerg_, expr)
#define emerg_err_sif(expr)             msg_err_sif(emerg_, expr)
#define emerg_err_ifm(expr, ...)        msg_err_ifm(emerg_, expr, __VA_ARGS__)
#define emerg_err_sifm(expr, ...)       msg_err_sifm(emerg_, expr, __VA_ARGS__)

#define emerg_goto_if(expr, gt)         msg_goto_if(emerg_, expr, gt)
#define emerg_strerror(err)             msg_strerror(emerg_, err)

/* alert_ macros */
#define alert(...)                      msg(alert_, 0, __VA_ARGS__) 
#define alert_err(...)                  msg_err(alert_, __VA_ARGS__)
#define alert_ifb(expr)                 msg_ifb(alert_, expr)
#define alert_if(expr)                  msg_if(alert_, expr) 
#define alert_ifm(expr, ...)            msg_ifm(alert_, expr, __VA_ARGS__) 

#define alert_return_if(expr, err)      msg_return_if(alert_, expr, err)
#define alert_return_sif(expr, err)     msg_return_sif(alert_, expr, err)
#define alert_return_ifm(expr, err, ...)      \
    msg_return_ifm(alert_, expr, err, __VA_ARGS__)
#define alert_return_sifm(expr, err, ...)      \
    msg_return_sifm(alert_, expr, err, __VA_ARGS__)

#define alert_err_if(expr)              msg_err_if(alert_, expr)
#define alert_err_sif(expr)             msg_err_sif(alert_, expr)
#define alert_err_ifm(expr, ...)        msg_err_ifm(alert_, expr, __VA_ARGS__)
#define alert_err_sifm(expr, ...)       msg_err_sifm(alert_, expr, __VA_ARGS__)

#define alert_goto_if(expr, gt)         msg_goto_if(alert_, expr, gt)
#define alert_strerror(err)             msg_strerror(alert_, err)

/* crit_ macros */
#define crit(...)                       msg(crit_, 0, __VA_ARGS__) 
#define crit_err(...)                   msg_err(crit_, __VA_ARGS__)
#define crit_ifb(expr)                  msg_ifb(crit_, expr)
#define crit_if(expr)                   msg_if(crit_, expr) 
#define crit_ifm(expr, ...)             msg_ifm(crit_, expr, __VA_ARGS__) 

#define crit_return_if(expr, err)       msg_return_if(crit_, expr, err)
#define crit_return_sif(expr, err)      msg_return_sif(crit_, expr, err)
#define crit_return_ifm(expr, err, ...) \
    msg_return_ifm(crit_, expr, err, __VA_ARGS__)
#define crit_return_sifm(expr, err, ...)    \
    msg_return_sifm(crit_, expr, err, __VA_ARGS__)

#define crit_err_if(expr)               msg_err_if(crit_, expr)
#define crit_err_sif(expr)              msg_err_sif(crit_, expr)
#define crit_err_ifm(expr, ...)         msg_err_ifm(crit_, expr, __VA_ARGS__)
#define crit_err_sifm(expr, ...)        msg_err_sifm(crit_, expr, __VA_ARGS__)

#define crit_goto_if(expr, gt)          msg_goto_if(crit, expr, gt)
#define crit_strerror(err)              msg_strerror(crit, err)

/* err_ macros */
#define err(...)                        msg(err_, 0, __VA_ARGS__)
#define err_err(...)                    msg_err(err_, __VA_ARGS__)
#define err_ifb(expr)                   msg_ifb(err_, expr)
#define err_if(expr)                    msg_if(err_, expr) 
#define err_ifm(expr, ...)              msg_ifm(err_, expr, __VA_ARGS__) 

#define err_return_if(expr, err)        msg_return_if(err_, expr, err)
#define err_return_sif(expr, err)       msg_return_sif(err_, expr, err)
#define err_return_ifm(expr, err, ...)  \
    msg_return_ifm(err_, expr, err, __VA_ARGS__)
#define err_return_sifm(expr, err, ...) \
    msg_return_sifm(err_, expr, err, __VA_ARGS__)

#define err_err_if(expr)                msg_err_if(err_, expr)
#define err_err_sif(expr)               msg_err_sif(err_, expr)
#define err_err_ifm(expr, ...)          msg_err_ifm(err_, expr, __VA_ARGS__)
#define err_err_sifm(expr, ...)         msg_err_sifm(err_, expr, __VA_ARGS__)

#define err_goto_if(expr, gt)           msg_goto_if(err_, expr, gt)
#define err_strerror(err)               msg_strerror(err_, err)

/* warn_ macros */
#define warn(...)                       msg(warn_, 0, __VA_ARGS__)
#define warn_err(...)                   msg_err(warn_, __VA_ARGS__)
#define warn_ifb(expr)                  msg_ifb(warn_, expr)
#define warn_if(expr)                   msg_if(warn_, expr) 
#define warn_ifm(expr, ...)             msg_ifm(warn_, expr, __VA_ARGS__) 

#define warn_return_if(expr, err)       msg_return_if(warn_, expr, err)
#define warn_return_sif(expr, err)      msg_return_sif(warn_, expr, err)
#define warn_return_ifm(expr, err, ...)      \
    msg_return_ifm(warn_, expr, err, __VA_ARGS__)
#define warn_return_sifm(expr, err, ...)      \
    msg_return_sifm(warn_, expr, err, __VA_ARGS__)

#define warn_err_if(expr)               msg_err_if(warn_, expr)
#define warn_err_sif(expr)              msg_err_sif(warn_, expr)
#define warn_err_ifm(expr, ...)         msg_err_ifm(warn_, expr, __VA_ARGS__)
#define warn_err_sifm(expr, ...)        msg_err_sifm(warn_, expr, __VA_ARGS__)

#define warn_goto_if(expr, gt)          msg_goto_if(warn_, expr, gt)
#define warn_strerror(err)              msg_strerror(warn_, err)

/* notice_ macros */
#define notice(...)                     msg(notice_, 0, __VA_ARGS__)
#define notice_err(...)                 msg_err(notice_, __VA_ARGS__)
#define notice_ifb(expr)                msg_ifb(notice_, expr)
#define notice_if(expr)                 msg_if(notice_, expr) 
#define notice_ifm(expr, ...)           msg_ifm(notice_, expr, __VA_ARGS__) 

#define notice_return_if(expr, err)     msg_return_if(notice_, expr, err)
#define notice_return_sif(expr, err)    msg_return_sif(notice_, expr, err)
#define notice_return_ifm(expr, err, ...)   \
    msg_return_ifm(notice_, expr, err, __VA_ARGS__)
#define notice_return_sifm(expr, err, ...)  \
    msg_return_sifm(notice_, expr, err, __VA_ARGS__)

#define notice_err_if(expr)             msg_err_if(notice_, expr)
#define notice_err_sif(expr)            msg_err_sif(notice_, expr)
#define notice_err_ifm(expr, ...)         \
    msg_err_ifm(notice_, expr, __VA_ARGS__)
#define notice_err_sifm(expr, ...)        \
    msg_err_sifm(notice_, expr, __VA_ARGS__)

#define notice_goto_if(expr, gt)        msg_goto_if(notice_, expr, gt)
#define notice_strerror(err)            msg_strerror(notice_, err)

/* info_ macros */
#define info(...)                       msg(info_, 0, __VA_ARGS__)
#define info_err(...)                   msg_err(info_, __VA_ARGS__)
#define info_ifb(expr)                  msg_ifb(info_, expr)
#define info_if(expr)                   msg_if(info_, expr) 
#define info_ifm(expr, ...)             msg_ifm(info_, expr, __VA_ARGS__) 

#define info_return_if(expr, err)       msg_return_if(info_, expr, err)
#define info_return_sif(expr, err)      msg_return_sif(info_, expr, err)
#define info_return_ifm(expr, err, ...)      \
    msg_return_ifm(info_, expr, err, __VA_ARGS__)
#define info_return_sifm(expr, err, ...)      \
    msg_return_sifm(info_, expr, err, __VA_ARGS__)

#define info_err_if(expr)               msg_err_if(info_, expr)
#define info_err_sif(expr)              msg_err_sif(info_, expr)
#define info_err_ifm(expr, ...)         msg_err_ifm(info_, expr, __VA_ARGS__)
#define info_err_sifm(expr, ...)        msg_err_sifm(info_, expr, __VA_ARGS__)

#define info_goto_if(expr, gt)          msg_goto_if(info_, expr, gt)
#define info_strerror(err)              msg_strerror(info_, err)

/* dbg_ macros */
#ifdef DEBUG
    #define dbg(...)                    msg(dbg_, 0, __VA_ARGS__)
    #define dbg_err(...)                msg_err(dbg_, __VA_ARGS__)

    #define dbg_if(expr)                msg_if(dbg_, expr) 
    #define dbg_ifb(expr)               msg_ifb(dbg_, expr)
    #define dbg_ifm(expr, ...)          msg_ifm(dbg_, expr, __VA_ARGS__) 

    #define dbg_return_if(expr, err)    msg_return_if(dbg_, expr, err)
    #define dbg_return_sif(expr, err)   msg_return_sif(dbg_, expr, err)
    #define dbg_return_ifm(expr, err, ...)      \
        msg_return_ifm(dbg_, expr, err, __VA_ARGS__)
    #define dbg_return_sifm(expr, err, ...)      \
        msg_return_sifm(dbg_, expr, err, __VA_ARGS__)

    #define dbg_err_if(expr)            msg_err_if(dbg_, expr)
    #define dbg_err_sif(expr)           msg_err_sif(dbg_, expr)
    #define dbg_err_ifm(expr, ...)      msg_err_ifm(dbg_, expr, __VA_ARGS__)
    #define dbg_err_sifm(expr, ...)     msg_err_sifm(dbg_, expr, __VA_ARGS__)

    #define dbg_goto_if(expr, gt)       msg_goto_if(dbg_, expr, gt)
    #define dbg_strerror(err)           msg_strerror(dbg_, err)

    /* simple debugging timing macros */
    #define TIMER_ON \
        time_t _t_beg = time(0), _t_prev = _t_beg, _t_now; int _t_step = 0
    #define TIMER_STEP                                                      \
        do {                                                                \
        _t_now = time(0);                                                   \
        dbg("  step %u: %u s (delta: %u s)",                                \
            _t_step++, _t_now - _t_beg, _t_now - _t_prev);                  \
            _t_prev = _t_now;                                               \
       } while(0)
    #define TIMER_OFF  dbg("elapsed: %u s", (time(0) - _t_beg))
#else /* disable debugging */
    #include <ctype.h>
    /* this will be used just to avoid empty-if (and similar) warnings */
    #define dbg_nop()                   isspace(0)
    #define dbg(...)                    dbg_nop()
    #define dbg_err(...)                do { goto err; } while(0)

    #define dbg_if(expr)                if( (expr) ) { ; }
    #define dbg_ifb(expr)               if( (expr) )
    #define dbg_ifm(expr, ...)          if( (expr) ) { ; }

    #define dbg_return_if(expr, err)    do { if( (expr) ) return err; } while(0)
    #define dbg_return_sif(expr, err)   dbg_return_if(expr, err)
    #define dbg_return_ifm(expr, err, ...)      \
        dbg_return_if(expr, err);
    #define dbg_return_sifm(expr, err, ...)     \
        dbg_return_if(expr, err);

    #define dbg_err_if(expr)            do { if( (expr) ) goto err; } while(0)
    #define dbg_err_sif(expr)           do { if( (expr) ) goto err; } while(0)
    #define dbg_err_ifm(expr, ...)      do { if( (expr) ) goto err; } while(0)
    #define dbg_err_sifm(expr, ...)     do { if( (expr) ) goto err; } while(0)

    #define dbg_goto_if(expr, gt)       do { if( (expr) ) goto gt; } while(0)
    #define dbg_strerror(err)           dbg_nop()
    #define TIMER_ON
    #define TIMER_STEP
    #define TIMER_OFF
#endif /* ifdef DEBUG */

#ifdef __cplusplus
}
#endif

#endif /* !_U_CARPAL_H_ */
