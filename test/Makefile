include common.mk
include ../Makefile.conf

PROG = runtest

SRCS += main.c
SRCS += misc.c 
SRCS += string.c 
SRCS += lexer.c 

ifndef NO_ARRAY
    SRCS += array.c 
endif
ifndef NO_LIST
    SRCS += list.c 
endif
ifndef NO_NET
    SRCS += uri.c
endif
ifndef NO_RB
    SRCS += rb.c
endif
ifndef NO_PWD
SRCS += pwd.c
endif
ifndef NO_HMAP
    SRCS += hmap.c 
endif
ifndef NO_PQUEUE
    SRCS += pqueue.c
endif
ifndef NO_BST
    SRCS += bst.c
endif
ifndef NO_JSON
    SRCS += json.c
    ifdef HAVE_ISFINITE
        LDFLAGS += -lm
    endif
endif

LDADD += ../srcs/libu.a

include prog.mk

ifndef NO_RUN_TEST
all-hook-post:
	@./$(PROG) -o - || echo "unit test execution failed"
endif
