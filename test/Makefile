include common.mk
include ../Makefile.conf

PROG = runtest
SRCS = main.c

SRCS += misc.c 
SRCS += hmap.c 
SRCS += list.c 
SRCS += string.c 
SRCS += array.c 

CFLAGS += -I../include
CFLAGS += -g -DDEBUG

LDADD += ../srcs/libu.a

include prog.mk
