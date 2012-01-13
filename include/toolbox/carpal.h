/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
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

/**
    \defgroup carpal Flow Control
    \{
        The \ref carpal module is at the very core of LibU.  It defines a 
        number of macros that are used throughout the library code to guide
        the execution flow.  They enforce an assertive programming style
        in which the code in the function body flows straight to the expected
        result and all the anomalies catched in the trunk are handled in a 
        a special \c err: labelled branch.  Each function body can be split
        into the following three blocks.
        <p>
        A declaration block:
    \code
    int f (void)
    {
        // local variables initialization
        char *d = NULL;
        FILE *fp = NULL;
    \endcode
        The trunk:
    \code
        // malloc a 1024 bytes 0-filled chunk
        dbg_err_sif ((d = u_zalloc(1024)) == NULL);

        // open "my.txt" for writing
        dbg_err_sif ((fp = fopen("my.txt", "w")) == NULL);

        // write to file
        dbg_err_sif (fwrite(d, 1024, 1, fp) < 1);

        // free memory and close the file stream 
        u_free(d);
        (void) fclose(fp);

        return 0;
    \endcode
        The exception handling branch:
    \code
    err:
        // invoke local resource dtors only in case they have been
        // allocated in the trunk and tell the caller that we have failed
        if (fp != NULL)
            fclose(fp);
        if (d != NULL)
            u_free(d);

        return ~0;
    }
    \endcode

        Should the execution fail in the function trunk, a number of 
        interesting info about the failed instruction and context are 
        automatically supplied to the syslog machinery and redirected 
        to the appropriate sink:
    \code
    Jan 15 12:27:27 tho-2 ./carp[14425]: [dbg][14425:main.c:20:f] (fp = fopen("my.txt", "w")) == ((void *)0) [errno: 13, Permission denied]
    \endcode

        As you can see, these macros also show the nice side effect of neatly 
        packing your code -- the LOC compression factor being nearly 1:3 
        compared to a properly formatted C program -- which of course increases 
        readability and maintainability of the software, but, most of all, 
        tries to protect the professional programmer from the nasty carpal 
        tunnel syndrome (hence the header name carpal.h).
        This is no doubt a clear case in which "Go To Statement can be 
        Considered Benefical" -- if Dijkstra allows.

        In all the macros that follow, the \c msg_ prefix can be substituted by
        any of \c dbg_, \c info_, \c notice_, \c warn_, \c err_, \c crit_,
        \c alert_, \c emerg_ strings, and the \p label argument deleted, since
        it is absorbed into the explicit prefix.  
        As you may easily spot, each of these variant is in 1:1 correspondence 
        with \c syslog(1) error levels.  
        Also, the special \c con_ prefix can be used to redirect the message 
        to \c stderr and \c nop_ to just don't log.
 */

/** 
 *  \brief Log a message and jump to the \c err: label
 *
 *  Log a message of type \p label and jump to \c err label
 *
 *  For example:
 *  \code
 *      if (stat(filename, &sb) == -1)
 *          dbg_err("stat failed on file %s", filename);
 *  \endcode
 */
#define msg_err(label, ...) \
    do { msg(label, 0, __VA_ARGS__); goto err; } while(0)

/** 
 *  \brief Log a message if \p expr is true
 *
 *  Log a message of type \p label if \p expr is true.  The \p expr text 
 *  statement will be written to the log file.
 *
 *  For example: 
 *  \code
 *      warn_if (check(abc) < 0);
 *  \endcode
 */
#define msg_if(label, expr) \
    do { if( expr ) msg_noargs(label, 0, #expr); } while(0)

/** 
 *  \brief  Log the given message if \p expr is true
 *
 *  Log the given message of type \p label if \e expr is true.
 *
 *  For example: 
 *  \code
 *      warn_ifm (check(abc) < 0, "check failed on file %s", filename);
 *  \endcode
 */
#define msg_ifm(label, expr, ...)   \
    do { if(expr) { msg(label, 0, __VA_ARGS__); } } while(0);

/** 
 *  \brief  Log a message if \p expr is true and enter the if-block
 *
 *  Log a message of type \p label if \p expr is true and enter the
 *  following if-block.  The \p expr text statement will be written 
 *  to the log file.
 *
 *  A C-style code block should follow. For example:
 *  \code
 *      dbg_ifb (i == 0)
 *      {
 *          statement;
 *          another_statement;
 *      }
 *  \endcode
 */
#define msg_ifb(label, expr)    \
    if( (expr) && (msg_noargs(label, 0, #expr) ? 1 : 1) ) 

/** 
 *  \brief  Log a message if \p expr is true and \c return with \e err.
 *
 *  Log a message if \p expr is true and \c return with \p err to the caller.
 *  The \p expr text statement will be written to the log file.
 *
 *  For example:
 *  \code
 *      // return with PRM_ERROR if input parameter validation fails
 *      dbg_return_if (param1 == NULL, PRM_ERROR);
 *  \endcode
 */
#define msg_return_if(label, expr, err) \
    msg_ifb(label, expr) { return err; }

/** 
 *  \brief  Log the given message if \p expr is true and return \p err.
 *
 *  Log the given message if \p expr is true and return \p err to the caller.
 *
 *  For example:
 *  \code
 *      dbg_return_ifm (prm == NULL, PRM_ERROR, "param %d must be not NULL", i);
 *  \endcode
 */
#define msg_return_ifm(label, expr, err, ...)   \
    if(expr) { msg(label, 0, __VA_ARGS__); return err; }

/** 
 *  \brief  Log the given message plus \c strerror(errno) if \p expr is true,
 *          and return \p err.
 *
 *  Log the given message plus \c strerror(errno) if \p expr is true, and 
 *  return \p err to the caller.
 *
 *  For example:
 *  \code
 *      dbg_return_sifm (stat(fn, &st) == -1, -1, "file %s access error", fn);
 *  \endcode
 */
#define msg_return_sifm(label, expr, err, ...)  \
    if(expr) { msg(label, errno, __VA_ARGS__); return err; }

/** 
 *  \brief  Log a message plus \c strerror(errno) if \p expr is true, and 
 *          return \p err. 
 *
 *  Log a message plus \c strerror(errno) if \p expr is true, and return 
 *  \p err to the caller.  The \p expr text statement will be written to 
 *  the log file.
 *
 *  For example:
 *  \code
 *      // return with ~0 and log errno returned by stat(2)
 *      warn_return_sif (stat(filename, &sb) == -1, ~0);
 *  \endcode
 */
#define msg_return_sif(label, expr, err)    \
    do { if(expr) { msg_noargs(label, errno, #expr); return err; } } while(0)

/** 
 *  \brief  Log a message if \p expr is true and \c goto \p gt.
 *
 *  Log a message of type \p label if \p expr is not zero and \c goto to the 
 *  label \p gt (that must be in-scope).  The \p expr text statement will be 
 *  written to the log file.
 *
 *  For example:
 *  \code
 *      again:
 *          ad = accept(sd, addr, addrlen);
 *          nop_goto_if (ad == -1 && errno == EINTR, again);
 *  \endcode
 */
#define msg_goto_if(label, expr, gt)    \
    msg_ifb(label, expr) goto gt

/** 
 *  \brief  Log a message if \p expr is true and \c goto to the label \c err
 *
 *  Log a message of type \p label if \p expr is true and \c goto to the 
 *  label \c err (that must be in-scope).  The \p expr text statement will 
 *  be written to the log file.
 *
 *  For example:
 *  \code
 *      dbg_err_if ((rc = regexec(&re, uri, 10, pmatch, 0)) != 0);
 *      ...
 *  err:
 *      u_dbg("no match found !");
 *      return ~0;
 *  \endcode
 */
#define msg_err_if(label, expr) \
    do { msg_ifb(label, expr) { goto err;} } while(0)

/** 
 *  \brief  Log a message if \p expr is true, set \p errcode and \c goto to 
 *          the label \c err
 *
 *  Log a message of type \p label if \p expr is true, set variable \c rc
 *  to \p errcode and jump to label \c err.  Both \c rc and \c err must be
 *  in-scope.  The \p expr text statement will be written to the log file.
 *
 *  For example:
 *  \code
 *  int rc = ERR_NONE;
 *  ...
 *  dbg_err_rcif ((madp = u_zalloc(UINT_MAX)) == NULL, ERR_MEM);
 *  ...
 *  err:
 *      return rc;
 *  \endcode
 */
#define msg_err_rcif(label, expr, errcode)  \
    do { msg_ifb(label, expr) { rc = errcode; goto err;} } while(0)

/** 
 *  \brief  Log the given message if \p expr is true and \c goto to the
 *          label \c err
 *
 *  Log a message of type \p label if \p expr is true and \c goto to the 
 *  label \c err (that must be in-scope).  The user-provided message is
 *  used instead of the \p expr text statement.
 *
 *  For example:
 *  \code
 *      dbg_err_ifm (cur == s, "empty IPv4address / reg-name");
 *      ...
 *  err:
 *      return ~0;
 *  \endcode
 */
#define msg_err_ifm(label, expr, ...)   \
    do { if( (expr) ) { msg(label, 0, __VA_ARGS__); goto err; } } while(0)

/** 
 *  \brief  Log a message and \c strerror(errno) if \p expr is true, then 
 *          \c goto to the label \c err
 *
 *  Log a message of type \p label if \p expr is true and \c goto
 *  to the label \c err (that must be in-scope).  Also logs \c strerror(errno).
 *
 *  For example:
 *  \code
 *      dbg_err_sif ((s = u_strdup(huge_string)) == NULL);
 *      ...
 *  err:
 *      return ~0;
 *  \endcode
 */
#define msg_err_sif(label, expr)    \
    do { if(expr) { msg_noargs(label, errno, #expr); goto err; } } while(0)

/** 
 *  \brief  Log the user provided message and \c strerror(errno) if \p expr 
 *          is true, then \c goto to the label \c err
 *
 *  Log the user provided message of type \p label if \p expr is true, log
 *  \c strerror(errno) and \c goto to the label \c err (that must be in-scope). 
 *
 *  For example:
 *  \code
 *      dbg_err_sifm ((fp = fopen(path, "r")) == NULL, "%s", path);
 *      ...
 *  err:
 *      return ~0;
 *  \endcode
 */
#define msg_err_sifm(label, expr, ...)  \
    do { if((expr)) { msg(label, errno, __VA_ARGS__);  goto err; } } while(0)


/** 
 *  \brief  Write a debug message containing the message returned by 
 *          \c strerror(errno) 
 *
 *  For example:
 *  \code
 *      switch (inet_pton(AF_INET6, host, &sin6->sin6_addr))
 *      {
 *          case -1:
 *              dbg_strerror(errno);
 *              // fall through
 *      ...
 *  \endcode
 */
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
#else   /* !OS_WIN */
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
#endif /* OS_WIN */

/**
 *  \}
 */  

/* nop_ macros */
#define nop_return_if(expr, err)       do { if(expr) return err; } while(0)
#define nop_err_if(expr)               do { if(expr) goto err;   } while(0)
#define nop_goto_if(expr, gt)          do { if(expr) goto gt;    } while(0)

/* con_ macros */
#define u_con(...)                      msg(con_, 0, __VA_ARGS__)
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
#define con_err_rcif(expr, ...)         msg_err_rcif(con_, expr, __VA_ARGS__)

#define con_goto_if(expr, gt)           msg_goto_if(con_, expr, gt)
#define con_strerror(err)               msg_strerror(con_, err)

/* emerg_ macros */
#define u_emerg(...)                    msg(emerg_, 0, __VA_ARGS__)
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
#define emerg_err_rcif(expr, ...)       msg_err_rcif(emerg_, expr, __VA_ARGS__)

#define emerg_goto_if(expr, gt)         msg_goto_if(emerg_, expr, gt)
#define emerg_strerror(err)             msg_strerror(emerg_, err)

/* alert_ macros */
#define u_alert(...)                    msg(alert_, 0, __VA_ARGS__) 
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
#define alert_err_rcif(expr, ...)       msg_err_rcif(alert_, expr, __VA_ARGS__)

#define alert_goto_if(expr, gt)         msg_goto_if(alert_, expr, gt)
#define alert_strerror(err)             msg_strerror(alert_, err)

/* crit_ macros */
#define u_crit(...)                     msg(crit_, 0, __VA_ARGS__) 
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
#define crit_err_rcif(expr, ...)        msg_err_rcif(crit_, expr, __VA_ARGS__)

#define crit_goto_if(expr, gt)          msg_goto_if(crit, expr, gt)
#define crit_strerror(err)              msg_strerror(crit, err)

/* err_ macros */
#define u_err(...)                      msg(err_, 0, __VA_ARGS__)
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
#define err_err_rcif(expr, ...)         msg_err_rcif(err_, expr, __VA_ARGS__)

#define err_goto_if(expr, gt)           msg_goto_if(err_, expr, gt)
#define err_strerror(err)               msg_strerror(err_, err)

/* warn_ macros */
#define u_warn(...)                     msg(warn_, 0, __VA_ARGS__)
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
#define warn_err_rcif(expr, ...)        msg_err_rcif(warn_, expr, __VA_ARGS__)

#define warn_goto_if(expr, gt)          msg_goto_if(warn_, expr, gt)
#define warn_strerror(err)              msg_strerror(warn_, err)

/* notice_ macros */
#define u_notice(...)                   msg(notice_, 0, __VA_ARGS__)
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
#define notice_err_rcif(expr, ...)        \
    msg_err_rcif(notice_, expr, __VA_ARGS__)

#define notice_goto_if(expr, gt)        msg_goto_if(notice_, expr, gt)
#define notice_strerror(err)            msg_strerror(notice_, err)

/* info_ macros */
#define u_info(...)                     msg(info_, 0, __VA_ARGS__)
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
#define info_err_rcif(expr, ...)        msg_err_rcif(info_, expr, __VA_ARGS__)

#define info_goto_if(expr, gt)          msg_goto_if(info_, expr, gt)
#define info_strerror(err)              msg_strerror(info_, err)

/* dbg_ macros */
#ifdef DEBUG
    #define u_dbg(...)                  msg(dbg_, 0, __VA_ARGS__)
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
    #define dbg_err_rcif(expr, ...)     msg_err_rcif(dbg_, expr, __VA_ARGS__)

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
    #define u_dbg(...)                  dbg_nop()
    #define dbg_err(...)                do { goto err; } while(0)

    #define dbg_if(expr)                do { if( (expr) ) { ; } } while(0)
    #define dbg_ifb(expr)               if( (expr) )
    #define dbg_ifm(expr, ...)          do { if( (expr) ) { ; } } while(0)

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
    #define dbg_err_rcif(expr, errcode, ...)    \
        do { if( (expr) ) { rc = errcode; goto err; } } while(0)

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
