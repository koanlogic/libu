# $Id: Makefile,v 1.6 2008/06/25 07:07:20 tho Exp $

SUBDIR += include 
SUBDIR += srcs 

ifndef NO_LIBU_DOCS
SUBDIR += doc
endif

ifdef LIBU_TEST
SUBDIR += doc
endif

include subdir.mk
