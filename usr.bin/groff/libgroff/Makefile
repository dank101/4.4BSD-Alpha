#	@(#)Makefile	6.7 (Berkeley) 7/18/92
#		rm -f ${DESTDIR}/usr/include/g++/groff/$$i; \

.include "../Makefile.g++"
.include "../Makefile.fontdir"

LIB=		groff
SRCS=		assert.cc change_lf.cc cmap.cc cset.cc errarg.cc \
		error.cc fatal.cc filename.cc font.cc fontfile.cc \
		iftoa.cc itoa.cc lf.cc lineno.cc nametoindex.cc \
		new.cc prime.cc progname.cc ptable.cc string.cc strsave.cc \
		tmpfile.cc version.cc
FILES=		assert.h cmap.h cset.h errarg.h error.h font.h \
		getpagesize.h lib.h ptable.h  stringclass.h
GXXFLAGS+=	-I$(.CURDIR) -DFONTPATH=\"$(FONTDIR)\"
NOMAN=		noman
NOPROFILE=	noprofile

beforeinstall:
	@echo installing ${FILES}
	@-for i in ${FILES}; do \
		cd ${.CURDIR}; \
		cmp -s $$i ${DESTDIR}/usr/include/g++/groff/$$i || \
		install -c -o ${BINOWN} -g ${BINGRP} -m ${MANMODE} \
			${.CURDIR}/$$i ${DESTDIR}/usr/include/g++/groff/$$i; \
	done

.include <bsd.lib.mk>

#
# For some reason, pmake won't cut it without this redundant declaration.
#
.SUFFIXES:
.SUFFIXES: .out .o .po .s .c .f .y .cc .l .8 .7 .6 .5 .4 .3 .2 .1 .0
