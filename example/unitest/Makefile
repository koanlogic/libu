include common.mk
include ../../Makefile.conf

PROG = test
SRCS = main.c
SRCS += test.c

LDADD += $(SRCDIR)/srcs/libu.a
CFLAGS += -fstack-protector

include prog.mk
