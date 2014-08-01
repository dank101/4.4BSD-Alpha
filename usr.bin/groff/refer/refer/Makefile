#
# Copyright (c) 1991 The Regents of the University of California.
# All rights reserved.
#
# %sccs.include.redist.sh
#
#	@(#)Makefile	6.2 (Berkeley) 11/13/91
#

.include "../Makefile.paths"
.include "../../Makefile.g++"
.include "../../../Makefile.inc"

PROG=		refer
SRCS=		refer.cc ref.cc token.cc search.cc linear.cc index.cc \
			label.y command.cc common.cc
LDADD+=		$(.CURDIR)/../../libgroff/obj/libgroff.a
.PATH:		$(.CURDIR)/../common_source

GXXFLAGS+=	-I$(.CURDIR) -I$(.CURDIR)/../common_source \
		-DCOMMON_WORDS_FILE=\"$(COMMON_WORDS_FILE)\" \
		-DDEFAULT_INDEX_DIR=\"$(DEFAULT_INDEX_DIR)\" \
		-DDEFAULT_INDEX_NAME=\"$(DEFAULT_INDEX_NAME)\" \
		-DINDEX_SUFFIX='$(INDEX_SUFFIX)'

.include <bsd.prog.mk>
