#	@(#)Makefile	5.1 (Berkeley) 5/3/91

PROG=	newfs
SRCS=	dkcksum.c newfs.c mkfs.c
CFLAGS+=-DLETS_GET_SMALL -DMFS
NOMAN=	noman
LDFLAGS=-N
.PATH:	${.CURDIR}/../../../../sbin/newfs ${.CURDIR}/../../../../sbin/disklabel

.include <bsd.prog.mk>
