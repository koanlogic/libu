/* 
 * Copyright (c) 2005, KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_LOGCFG_H_
#define _U_LOGCFG_H_

#include <syslog.h>

/** \file */

/** \brief Log configuration 
 *
 * Modify this enum (except LF_MISC) to associate an application facility 
 * to a syslogd facility
 */
enum {
    LF_MISC      = LOG_LOCAL0,  /* used internally (DO NOT MODIFY) */
    LF_SPARE1    = LOG_LOCAL1,
    LF_SPARE2    = LOG_LOCAL2,
    LF_SPARE3    = LOG_LOCAL3,
    LF_SPARE4    = LOG_LOCAL4,
    LF_SPARE5    = LOG_LOCAL5,
    LF_SPARE6    = LOG_LOCAL6,
    LF_SPARE7    = LOG_LOCAL7
};

#endif /* !_U_LOGCFG_H_ */
