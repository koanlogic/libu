# $Id: Makefile,v 1.2 2007/06/04 16:13:28 tat Exp $

include common.mk
include ../../Makefile.conf

INCS = libu.h missing.h toolbox.h libu_conf.h

include incs.mk

all-hook-pre:
	rm -rf missing && ln -s ../missing
	rm -rf toolbox && ln -s ../toolbox

clean-hook-pre:
	rm -rf missing toolbox
