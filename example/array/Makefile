# $Id: Makefile,v 1.1 2008/06/24 12:00:24 tho Exp $

include common.mk

PROG = dynarray
SRCS = main.c

CFLAGS += -ggdb -DDEBUG
CFLAGS += -W -Wall
CFLAGS += -I../../include
#LDFLAGS += -L../../srcs -lu
LDADD += ../../srcs/libu.a

include prog.mk
