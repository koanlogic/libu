# $Id: Makefile,v 1.17 2010/01/14 12:28:29 tho Exp $

include common.mk
include ../Makefile.conf

LIB = u

# toolbox core sources
SRCS =  toolbox/memory.c
SRCS += toolbox/misc.c
SRCS += toolbox/str.c
SRCS += toolbox/buf.c
SRCS += toolbox/log.c
SRCS += toolbox/test.c

ifndef NO_HMAP
    SRCS += toolbox/hmap.c
endif
ifndef NO_CONFIG
    SRCS += toolbox/config.c
    SRCS += toolbox/config_fs.c
endif
ifndef NO_ENV
    SRCS += toolbox/env.c
endif
ifndef NO_NET
    SRCS += toolbox/net.c 
    SRCS += toolbox/uri.c
endif
ifndef NO_FS
    SRCS += toolbox/fs.c
endif
ifndef NO_PWD
    ifdef NO_HMAP   # pwd needs hmap
        $(warning adding hmap module as a dependency to pwd)
        SRCS += toolbox/hmap.c
    endif   # NO_HMAP (PWD dep)
    SRCS += toolbox/pwd.c
endif   # !NO_PWD 
ifndef NO_LIST
    SRCS += toolbox/list.c
endif
ifndef NO_ARRAY
    SRCS += toolbox/array.c
endif
ifndef NO_RB
    SRCS += toolbox/rb.c
endif
ifdef SHLIB_NO_UNDEFINED_SYMS
    SRCS += toolbox/facility.c
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
ifndef HAVE_MKSTEMPS
    SRCS += missing/mkstemps.c
endif
ifndef HAVE_SETENV
    SRCS += missing/setenv.c
endif

include lib.mk

# MacOSX ld(1) bug workaround
ifdef OS_DARWIN
install-hook-post:
	chmod u+w $(LIBDIR)/libu.a && $(RANLIB) $(LIBDIR)/libu.a
endif
