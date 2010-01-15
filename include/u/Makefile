# $Id: Makefile,v 1.6 2010/01/15 17:47:56 tho Exp $

include common.mk
include ../../Makefile.conf

INCS = libu.h missing.h toolbox.h compat.h libu_conf.h

LINKS = missing toolbox

depend-hook-pre all-hook-pre: 
	for d in $(LINKS) ; do rm -f $$d && ln -s ../$$d ; done
 
clean-hook-pre: ; rm -rf missing toolbox

include incs.mk
