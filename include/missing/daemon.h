/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _LIBU_DAEMON_H_
#define _LIBU_DAEMON_H_

#include <u/libu_conf.h>

#ifdef HAVE_DAEMON
  #ifdef HAVE_STDLIB
    #include <stdlib.h>
  #endif
  #ifdef HAVE_UNISTD
    #include <unistd.h>
  #endif
#else   /* !HAVE_DAEMON */

#ifdef __cplusplus
extern "C" {
#endif

int daemon(int nochdir, int noclose);

#ifdef __cplusplus
}
#endif

#endif  /* HAVE_DAEMON */

#endif  /* !_LIBU_DAEMON_H_ */
