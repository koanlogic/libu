include common.mk
include ../Makefile.conf

PROG = runtest

CFLAGS += -fnested-functions

SRCS += main.c
SRCS += misc.c 
SRCS += string.c 

ifndef NO_HMAP
    SRCS += hmap.c 
endif

ifndef NO_LIST
    SRCS += list.c 
endif

ifndef NO_ARRAY
    SRCS += array.c 
endif

ifndef NO_NET
    SRCS += uri.c
endif

ifndef NO_PQUEUE
    SRCS += pqueue.c
endif

LDADD += ../srcs/libu.a

include prog.mk

ifndef NO_RUN_TEST
all-hook-post:
	@./$(PROG) -v || echo "unit test execution failed"
endif
