#	@(#)Makefile	6.3 (Berkeley) 11/13/91

.include "../Makefile.g++"
.include "../../Makefile.inc"
.include "../Makefile.device"

PROG=		eqn
SRCS=		box.cc delim.cc eqn.y lex.cc limit.cc list.cc \
		main.cc mark.cc other.cc over.cc pile.cc script.cc \
		special.cc sqrt.cc text.cc
LDADD+=		$(.CURDIR)/../libgroff/obj/libgroff.a
GXXFLAGS+=	-I. -I$(.CURDIR) -DDEVICE=\"$(DEVICE)\" \
		-DPSPRINT='$(PSPRINT)' \
		-DDVIPRINT='$(DVIPRINT)'
CLEANFILES+=	eqn.tab.h

.include <bsd.prog.mk>
