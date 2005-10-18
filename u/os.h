#ifndef _LIBU_OS_H_
#define _LIBU_OS_H_
#include <stdarg.h>
#include "conf.h"

#ifndef HAVE_STRTOK_R
char * strtok_r(char *s, const char *delim, char **last);
#else
#include <string.h>
#endif

#ifndef HAVE_UNLINK
int unlink(const char *pathname)
#else
#include <unistd.h>
#endif

#ifndef HAVE_GETPID
pid_t getpid();
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#ifndef HAVE_SYSLOG

/* level codes */
#define	LOG_EMERG	    0	
#define	LOG_ALERT	    1
#define	LOG_CRIT	    2
#define	LOG_ERR		    3
#define	LOG_WARNING	    4
#define	LOG_NOTICE	    5
#define	LOG_INFO	    6
#define	LOG_DEBUG	    7

/* facility codes */
#define	LOG_KERN	    (0<<3)
#define	LOG_USER	    (1<<3)
#define	LOG_MAIL	    (2<<3)
#define	LOG_DAEMON	    (3<<3)
#define	LOG_AUTH	    (4<<3)
#define	LOG_SYSLOG	    (5<<3)
#define	LOG_LPR		    (6<<3)
#define	LOG_NEWS	    (7<<3)
#define	LOG_UUCP	    (8<<3)
#define	LOG_CRON	    (9<<3)
#define	LOG_AUTHPRIV	(10<<3)
#define	LOG_FTP		    (11<<3)
#define	LOG_NETINFO	    (12<<3)
#define	LOG_REMOTEAUTH	(13<<3)
#define	LOG_INSTALL	    (14<<3)
#define	LOG_LOCAL0	    (16<<3)	
#define	LOG_LOCAL1	    (17<<3)
#define	LOG_LOCAL2	    (18<<3)
#define	LOG_LOCAL3	    (19<<3)
#define	LOG_LOCAL4	    (20<<3)
#define	LOG_LOCAL5	    (21<<3)
#define	LOG_LOCAL6	    (22<<3)
#define	LOG_LOCAL7	    (23<<3)

void syslog(int priority, const char *msg, ...);
#else
#include <syslog.h>
#include <stdarg.h>
#endif

#endif 

