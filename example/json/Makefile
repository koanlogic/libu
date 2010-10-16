# $Id: Makefile,v 1.2 2010/05/27 08:35:12 tho Exp $

include common.mk
include ../../Makefile.conf

PROG = json
SRCS = main.c 
#SRCS += json.c

#CFLAGS += -DJSON_LEX_DEBUG
#CFLAGS += -DJSON_OBJ_DEBUG

LDADD += $(SRCDIR)/srcs/libu.a
LDFLAGS += -lm

include prog.mk
