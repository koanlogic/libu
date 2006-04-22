/* 
 * Copyright (c) 2005, 2006 by KoanLogic s.r.l. - All rights reserved.  
 */
#ifndef _LIBU_OS_H_
#define _LIBU_OS_H_
#include "libu_conf.h"
#include <u/syslog.h>
#include <u/strtok_r.h>
#include <u/unlink.h>
#include <u/getpid.h>
#include <u/fnmatch.h>
#include <u/timegm.h>
#include <u/strsep.h>
#include <u/gettimeofday.h>
#include <u/daemon.h>

#ifdef OS_WIN
#include <windows.h>
#define strcasecmp _stricmp
#define sleep(secs) Sleep( (secs) * 1000 )
#endif

#ifndef HAVE_SSIZE_T
typedef int ssize_t;
#endif

/* on VxWorks/DCC there's not extern declaration (even if the var is available
   into library files) */
extern char *optarg;
extern int optind;

#endif 
