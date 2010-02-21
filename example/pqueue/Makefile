include common.mk
include ../../Makefile.conf

PROG = pqueue
SRCS = main.c
SRCS += pqueue.c

LDADD += $(SRCDIR)/srcs/libu.a
CFLAGS += -fstack-protector
CFLAGS += -ggdb

include prog.mk
