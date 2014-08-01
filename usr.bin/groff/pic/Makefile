#	@(#)Makefile	6.4 (Berkeley) 7/18/92

.include "../Makefile.g++"
.include "../../Makefile.inc"

PROG=		pic
SRCS=           common.cc main.cc object.cc \
		pic.y lex.cc tex.cc troff.cc

LDADD+=		$(.CURDIR)/../libgroff/obj/libgroff.a
# Delete the -Ig++-include after we fix libg++ and install it
GXXFLAGS+=	-I. -I$(.CURDIR)

CLEANFILES+=	pic.tab.h

.include <bsd.prog.mk>
