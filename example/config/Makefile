# $Id: Makefile,v 1.1 2008/04/30 16:12:54 tat Exp $

include common.mk

PROG = uconfig
SRCS = main.c

CFLAGS += -ggdb -DDEBUG
CFLAGS += -W -Wall -Wextra
CFLAGS += -I../../include
#LDFLAGS += -L../../srcs -lu
LDADD += ../../srcs/libu.a

include prog.mk
