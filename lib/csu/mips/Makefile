#	@(#)Makefile	5.1 (Berkeley) 7/19/92

CFLAGS=	-O -DLIBC_SCCS
OBJS=	crt0.o
CLEANFILES+=	gmon.o mon.o moncrt0.o core a.out

all: ${OBJS}

crt0.o: crt0.s
	${CC} -c -DCRT0 ${.ALLSRC}

install:
	install -o ${BINOWN} -g ${BINGRP} -m 444 ${OBJS} \
	    ${DESTDIR}/usr/lib

depend lint tags:

.include <bsd.prog.mk>
