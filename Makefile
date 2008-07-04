# $Id: Makefile,v 1.8 2008/07/04 10:55:37 tho Exp $

include Makefile.conf

SUBDIR = include
SUBDIR += srcs 

ifndef NO_DOCS
SUBDIR += doc
endif

ifdef DO_TEST
SUBDIR += test
endif

include subdir.mk
