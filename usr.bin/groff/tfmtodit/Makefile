#
# Copyright (c) 1991 The Regents of the University of California.
# All rights reserved.
#
# %sccs.include.redist.sh
#
#	@(#)Makefile	6.3 (Berkeley) 11/13/91
#

.include "../Makefile.g++"

PROG=		tfmtodit
SRCS=		tfmtodit.cc
LDADD+=		$(.CURDIR)/../libgroff/obj/libgroff.a

afterinstall:
	echo install macros and devices

.include "../../Makefile.inc"
.include <bsd.prog.mk>
