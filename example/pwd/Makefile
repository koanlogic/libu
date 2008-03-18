# $Id: Makefile,v 1.2 2008/03/18 10:38:04 tho Exp $

include common.mk

PROG = pwd
SRCS = main.c

CFLAGS += -ggdb -DDEBUG
CFLAGS += -W -Wall
CFLAGS += -I../../include
#LDFLAGS += -L../../srcs -lu
LDADD += ../../srcs/libu.a

include prog.mk
