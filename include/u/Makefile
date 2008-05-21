# $Id: Makefile,v 1.5 2008/05/21 15:12:32 tho Exp $

include common.mk
include ../../Makefile.conf

INCS = libu.h missing.h toolbox.h libu_conf.h

LINKS = missing toolbox

depend-hook-pre all-hook-pre: 
	for d in $(LINKS) ; do rm -f $$d && ln -s ../$$d ; done
 
clean-hook-pre: ; rm -rf missing toolbox

include incs.mk
