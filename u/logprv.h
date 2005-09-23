/* 
 * Copyright (c) 2005, KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _U_LOGPRV_H_
#define _U_LOGPRV_H_

#include <stdarg.h>

#define u_log_write(fac, lev, ctx, args...) \
    u_log_write_ex(fac, lev, ctx, __FILE__, __LINE__,__FUNCTION__, args) 

/** pack context information (if [ctx] is > 0) and the priority label and 
    send both to the log system followed by the user message string 

  fac:  facility (label)
  lev:  priority level
  ctx:  prints context information if [ctx] > 0
  file: filename
  line: line number
  func: function name
  fmt:  printf-style format string
  ...:  optional parameters

  */
int u_log_write_ex(int fac, int lev, int ctx, const char* file, int line, 
    const char *func, const char* fmt, ...);

#endif /* !_U_LOGCFG_H_ */
