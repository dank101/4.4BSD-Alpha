#	@(#)Makefile	6.5 (Berkeley) 7/6/92

LIB=		gnulib
CC=		GCC_EXEC_PREFIX=$(.CURDIR)/../cpp/obj/ \
		$(.CURDIR)/../cc/obj/cc -B$(.CURDIR)/../cc1/obj/
CFLAGS+=	-I. -I$(.CURDIR)/../cc1 -I$(.CURDIR)/../cc1/config
NOMAN=		noman
.PATH: $(.CURDIR)/g++

GPLUS_SRCS=	builtin_New.c builtin_del.c builtin_new.c eprintf.c
SRCS=		$(MACHINE_SRCS) $(GPLUS_SRCS)

$(OBJS):	config.h

afterinstall:
	-rm -f $(DESTDIR)$(LIBDIR)/gcc-gnulib
	cd $(DESTDIR)$(LIBDIR); ln -s libgnulib.a gcc-gnulib

.include <bsd.lib.mk>
.include "../Makefile.symlinks"
