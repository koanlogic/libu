# $Id: Makefile,v 1.1.1.1 2005/09/23 13:04:38 tho Exp $

include ../../Makefile.conf

ifdef HAVE_DOXYGEN 

all:
	doxygen libu.doxy

install: all
	mkdir -p ${DESTDIR}/doc/html && cp -r html/ ${DESTDIR}/doc/

uninstall:
	rm -rf ${DESTDIR}/doc/html

clean:
	rm -rf html/

else

all clean install uninstall:

endif

depend cleandepend:
