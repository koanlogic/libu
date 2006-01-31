# $Id: Makefile,v 1.4 2006/01/31 17:08:13 tho Exp $

include common.mk
include ../../Makefile.conf

ifdef HAVE_DOXYGEN 

all: img
	doxygen libu.doxy

img:
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
