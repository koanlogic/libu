# $Id: Makefile.doxy.doc,v 1.7 2007/01/31 15:59:49 tat Exp $

include common.mk
include ../../Makefile.conf

HTMLDOCDIR = ${DOCDIR}/libu/html/

all clean depend cleandepend:

install:
	@if [ -f index.html ]; then \
	    ${MKINSTALLDIRS} ${HTMLDOCDIR} && cp -r *.html *.css ${HTMLDOCDIR} ; \
    fi

uninstall:
	rm -rf ${HTMLDOCDIR}
