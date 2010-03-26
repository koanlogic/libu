# $Id: Makefile,v 1.2 2010/03/26 21:02:58 tho Exp $

include common.mk
include ../../Makefile.conf

PROG = urb
SRCS = main.c

CFLAGS += -ggdb -DDEBUG
CFLAGS += -I../../include
LDADD += ../../srcs/libu.a

include prog.mk
