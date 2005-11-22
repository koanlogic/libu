/* 
 * Copyright (c) 2005, KoanLogic s.r.l. - All rights reserved.  
 */
#ifndef _LIBU_GETPID_H_
#define _LIBU_GETPID_H_
#include "libu_conf.h"

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

pid_t getpid(void);
#endif

#endif
