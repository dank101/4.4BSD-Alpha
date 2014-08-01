#	@(#)Makefile	6.2 (Berkeley) 11/13/91

.include "../Makefile.g++"

PROG=		grotty
SRCS=		grotty.cc
LDADD+=		$(.CURDIR)/../libdriver/obj/libdriver.a \
		$(.CURDIR)/../libgroff/obj/libgroff.a
GXXFLAGS+=	-I$(.CURDIR)/../libdriver

afterinstall:
	echo install macros and devices

.include "../../Makefile.inc"
.include <bsd.prog.mk>
