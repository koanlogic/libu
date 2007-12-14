include common.mk
include ../Makefile.conf

PROG = runtest
SRCS = main.c

SRCS += misc.c 
SRCS += hmap.c 

CFLAGS += -I../include
CFLAGS += -g -DDEBUG

LDADD += ../srcs/libu.a

include prog.mk
