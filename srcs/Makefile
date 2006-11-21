# $Id: Makefile,v 1.3 2006/11/21 08:54:49 tho Exp $

include common.mk
include ../Makefile.conf

LIB = u

# toolbox sources
SRCS = toolbox/memory.c \
       toolbox/misc.c   \
       toolbox/str.c    \
       toolbox/buf.c    \
       toolbox/log.c

ifndef NO_HMAP
SRCS += toolbox/hmap.c
endif
ifndef NO_CONFIG
SRCS += toolbox/config.c
endif
ifndef NO_ENV
SRCS += toolbox/env.c
endif
ifndef NO_NET
SRCS += toolbox/net.c toolbox/uri.c
endif

# missing sources
ifndef HAVE_SYSLOG
SRCS += missing/syslog.c
endif
ifndef HAVE_STRTOK_R
SRCS += missing/strtok_r.c
endif
ifndef HAVE_UNLINK
SRCS += missing/unlink.c
endif
ifndef HAVE_GETPID
SRCS += missing/getpid.c
endif
ifndef HAVE_FNMATCH
SRCS += missing/fnmatch.c
endif
ifndef HAVE_TIMEGM
SRCS += missing/timegm.c
endif
ifndef HAVE_STRSEP
SRCS += missing/strsep.c
endif
ifndef HAVE_STRLCPY
SRCS += missing/strlcpy.c
endif
ifndef HAVE_STRLCAT
SRCS += missing/strlcat.c
endif
ifndef HAVE_GETTIMEOFDAY
SRCS += missing/gettimeofday.c
endif
ifndef HAVE_DAEMON
SRCS += missing/daemon.c
endif

include lib.mk

# MacOSX ld(1) bug workaround
install-hook-post:
	$(RANLIB) $(LIBDIR)/libu.a

