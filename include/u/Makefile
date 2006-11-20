# $Id: Makefile,v 1.1 2006/11/20 13:36:12 tho Exp $

include common.mk
include ../../Makefile.conf

INCS = libu.h missing.h toolbox.h libu_conf.h

include incs.mk

all-hook-pre:
	rm -f missing && ln -s ../missing
	rm -f toolbox && ln -s ../toolbox

clean-hook-pre:
	rm -f missing toolbox
