# $Id: Makefile,v 1.2 2005/10/19 15:54:49 stewy Exp $

include ../../Makefile.conf

ifdef HAVE_DOXYGEN 

all: img
	doxygen libu.doxy

img:
	doxygen libu.doxy
	mkdir -p html/images
	cp -r images html

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
