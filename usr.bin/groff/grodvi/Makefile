# Copyright (c) 1991 The Regents of the University of California.
# All rights reserved.
#
# %sccs.include.redist.sh
#
#	@(#)Makefile	6.2 (Berkeley) 7/15/92
#

.include "../Makefile.g++"

PROG=		grodvi
SRCS=		dvi.cc
LDADD+=		$(.CURDIR)/../libdriver/obj/libdriver.a \
		$(.CURDIR)/../libgroff/obj/libgroff.a
GXXFLAGS+=	-I$(.CURDIR)/../libdriver

afterinstall:
	echo install macros and devices

.include "../../Makefile.inc"
.include <bsd.prog.mk>
