/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
 */

#ifndef _LIBU_GETPID_H_
#define _LIBU_GETPID_H_

#include <u/libu_conf.h>

#ifdef HAVE_GETPID
#include <sys/types.h>
#include <unistd.h>
#else /* !HAVE_GETPID */

#ifdef OS_WIN
    #include <windows.h>
    typedef DWORD pid_t;
#else /* !OS_WIN */
    typedef unsigned int pid_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

pid_t getpid(void);

#ifdef __cplusplus
}
#endif

#endif  /* HAVE_GETPID */

#endif  /* !_LIBU_GETPID_H_ */
