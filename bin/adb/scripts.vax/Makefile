#	@(#)Makefile	5.6 (Berkeley) 6/8/90

LIST=	dirblk dmcstats findproc.nxt frame ifuba ifubinfo ifxmt \
	mba_device mba_hd pcb proc setproc.nxt stacktrace trapframe \
	ubahd
NOOBJ=	noobj

all clean cleandir depend lint tags:

install:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${LIST} \
	    ${DESTDIR}/usr/libdata/adb

.include <bsd.prog.mk>
