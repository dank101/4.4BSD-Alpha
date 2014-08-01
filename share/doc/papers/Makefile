#	@(#)Makefile	5.1 (Berkeley) 6/30/90

BINDIR=	/usr/share/doc/papers
FILES=	Makefile
SUBDIR=	beyond4.3 diskperf fsinterface future gprof kernmalloc kerntune \
	memfs newvm px relengr sysperf

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}

.include <bsd.subdir.mk>
