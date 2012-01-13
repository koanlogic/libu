/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_LOGPRV_H_
#define _U_LOGPRV_H_

#include <u/libu_conf.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    LOG_WRITE_FLAG_NONE,
    LOG_WRITE_FLAG_CTX      /* include file:line in the output message */
};

#define u_log_write(fac, lev, flags, err, ...)                               \
    u_log_write_ex(fac, lev, flags, err, __FILE__, __LINE__, __FUNCTION__, \
            __VA_ARGS__)

#define u_console_write(err, ...) \
    u_console_write_ex(err, __FILE__, __LINE__, __FUNCTION__,  __VA_ARGS__)

int u_log_write_ex(int fac, int lev, int flags, int err, const char* file, 
    int line, const char *func, const char* fmt, ...);

int u_console_write_ex(int err, const char* file, int line, 
    const char *func, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* !_U_LOGPRV_H_ */
