# $Id: Makefile,v 1.7 2005/11/23 18:55:21 tat Exp $

include ../Makefile.conf

LIB = u
SRCS = config.c env.c log.c memory.c misc.c net.c str.c uri.c

ifndef HAVE_SYSLOG
SRCS += syslog.c
endif

ifndef HAVE_STRTOK_R
SRCS += strtok_r.c
endif

ifndef HAVE_UNLINK
SRCS += unlink.c
endif

ifndef HAVE_GETPID
SRCS += getpid.c
endif

ifndef HAVE_FNMATCH
SRCS += fnmatch.c
endif

ifndef HAVE_TIMEGM
SRCS += timegm.c
endif

ifndef HAVE_STRSEP
SRCS += strsep.c
endif

ifndef HAVE_GETTIMEOFDAY
SRCS += gettimeofday.c
endif

include lib.mk
