/* 
 * Copyright (c) 2005, KoanLogic s.r.l. - All rights reserved.  
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
#ifdef OS_WIN
#include <windows.h>
#define strcasecmp _stricmp
#endif
#endif 
