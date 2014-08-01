#	@(#)Makefile	6.4 (Berkeley) 11/13/91

.include "../Makefile.g++"
.include "../Makefile.device"

PROG=		groff
SRCS=		groff.cc
LDADD+=		$(.CURDIR)/../libgroff/obj/libgroff.a
GXXFLAGS+=	-DDEVICE=\"$(DEVICE)\" -DPSPRINT='$(PSPRINT)' \
		-DDVIPRINT='$(DVIPRINT)' -DHAVE_SYS_SIGLIST

afterinstall:
	install -c -o $(BINOWN) -g $(BINGRP) -m $(BINMODE) \
		$(.CURDIR)/nroff.sh $(DESTDIR)$(BINDIR)/nroff

# this doesn't do the right thing yet - need to add psroff-like flags
#	install -c -o $(BINOWN) -g $(BINGRP) -m $(BINMODE) \
#		$(.CURDIR)/psroff.sh $(DESTDIR)$(BINDIR)/psroff

.include "../../Makefile.inc"
.include <bsd.prog.mk>
