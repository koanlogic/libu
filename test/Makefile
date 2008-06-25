include common.mk
include ../Makefile.conf

PROG = runtest

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

LDADD += ../srcs/libu.a

include prog.mk

all-hook-post:
	@./$(PROG) || echo "unit test execution failed"
