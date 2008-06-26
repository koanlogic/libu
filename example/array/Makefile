# $Id: Makefile,v 1.2 2008/06/26 16:03:39 tho Exp $

include common.mk
include ../../Makefile.conf

PROG = dynarray
SRCS = main.c

CFLAGS += -ggdb -DDEBUG
CFLAGS += -W -Wall
CFLAGS += -I../../include
LDADD += ../../srcs/libu.a

include prog.mk
