#	@(#)Makefile	6.4 (Berkeley) 11/13/91

.include "../Makefile.g++"
.include "../Makefile.device"
.include "../../Makefile.inc"

PROG=		troff
HYPHENDIR=	/usr/share/misc
MACROPATH=	/usr/share/tmac
SRCS=		column.cc dictionary.cc div.cc env.cc input.cc \
		node.cc number.cc reg.cc symbol.cc
LDADD+=		$(.CURDIR)/../libgroff/obj/libgroff.a
GXXFLAGS+=	-I$(.CURDIR) -DMACROPATH=\"$(MACROPATH)\" -DSTORE_WIDTH \
		-DHYPHENFILE=\"$(HYPHENDIR)/hyphen\" \
		-DDEVICE=\"$(DEVICE)\"

afterinstall:
	install -c -o bin -g bin -m 444 $(.CURDIR)/hyphen \
		$(DESTDIR)$(HYPHENDIR)/hyphen

.include <bsd.prog.mk>
