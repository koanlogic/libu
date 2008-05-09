# $Id: Makefile,v 1.4 2008/05/09 07:58:15 tho Exp $

SUBDIR = include srcs doc
LINKS = missing toolbox

all-pre depend-pre: 
	(cd include/u && for d in $(LINKS) ; do rm -rf $$d && ln -s ../$$d; done)

clean-pre:
	(cd include/u && rm -rf $(LINKS))

include subdir.mk
