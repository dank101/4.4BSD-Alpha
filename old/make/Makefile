#	@(#)Makefile	4.12 (Berkeley) 5/22/91

PROG=	make
CFLAGS+=-DASCARCH -I${.CURDIR} -fwritable-strings
SRCS=	main.c doname.c misc.c files.c dosys.c gram.c
MAN1=	make.0
CLEANFILES=gram.c y.tab.h

.include <bsd.prog.mk>
