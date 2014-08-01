#	@(#)Makefile	6.2 (Berkeley) 11/13/91

.include "../Makefile.g++"
.include "../../Makefile.inc"

PROG=		tbl
SRCS=		main.cc table.cc
LDADD+=		$(.CURDIR)/../libgroff/obj/libgroff.a
GXXFLAGS+=	-I$(.CURDIR)

.include <bsd.prog.mk>
