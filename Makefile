# $Id: Makefile,v 1.7 2008/06/25 10:10:15 tho Exp $

include Makefile.conf

SUBDIR += include 
SUBDIR += srcs 

ifndef NO_DOCS
SUBDIR += doc
endif

ifdef DO_TEST
SUBDIR += test
endif

include subdir.mk
