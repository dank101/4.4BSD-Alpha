#	@(#)Makefile	1.2 (Berkeley) 7/15/92

TMACOWN?=	bin
TMACGRP?=	bin
TMACMODE?=	444
TMACDIR?=	/usr/share/tmac

TMACS=	tmac.X tmac.dvi tmac.pic tmac.ps tmac.psfig tmac.tty\
	tmac.groff_an tmac.groff_me tmac.groff_ms

all:

clean:

cleandir:

obj:

install:
	-for t in $(TMACS); do \
		install -c -o $(TMACOWN) -g $(TMACGRP) -m $(TMACMODE) \
			$(.CURDIR)/$$t $(DESTDIR)$(TMACDIR)/$$t; \
	done
