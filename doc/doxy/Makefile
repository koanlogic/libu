# $Id: Makefile,v 1.5 2006/11/07 09:15:31 tho Exp $

include common.mk
include ../../Makefile.conf

ifdef HAVE_DOXYGEN 

all:
	doxygen libu.doxy
	mkdir -p html/images
	cp -r images html

install: all
	mkdir -p ${DESTDIR}/doc/libu/ && cp -r html ${DESTDIR}/doc/libu/

uninstall:
	rm -rf ${DESTDIR}/doc/libu/

clean:
	rm -rf html/

else

all clean install uninstall:

endif

depend cleandepend:
