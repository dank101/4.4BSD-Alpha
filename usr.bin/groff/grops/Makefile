#	@(#)Makefile	6.3 (Berkeley) 11/13/91

.include "../Makefile.g++"

PROG=		grops
SRCS=		ps.cc psrm.cc

BROKEN_SPOOLER_FLAGS=0

LDADD+=		$(.CURDIR)/../libdriver/obj/libdriver.a \
		$(.CURDIR)/../libgroff/obj/libgroff.a
GXXFLAGS+=	-I$(.CURDIR)/../libdriver \
		-DBROKEN_SPOOLER_FLAGS=\"$(BROKEN_SPOOLER_FLAGS)\"

afterinstall:
	echo install macros and devices

.include "../../Makefile.inc"
.include <bsd.prog.mk>


