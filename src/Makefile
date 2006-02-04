# $Id: Makefile,v 1.11 2006/02/04 21:32:55 tho Exp $

include common.mk
include ../Makefile.conf

LIB = u
SRCS = memory.c misc.c str.c

ifndef NO_HMAP
SRCS += hmap.c
endif

ifndef NO_CONFIG
SRCS += config.c
endif

ifndef NO_ENV
SRCS += env.c
endif

ifndef NO_LOG
SRCS += log.c
endif

ifndef NO_NET
SRCS += net.c uri.c
endif

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
