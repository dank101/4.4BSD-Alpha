#	@(#)Makefile	5.3 (Berkeley) 7/24/92

M4=	m4
#M4=	/usr/src/usr.bin/m4/obj/m4
CHMOD=	chmod
ROMODE=	444
RM=	rm -f

.SUFFIXES:  .mc .cf

.mc.cf:
	$(RM) $@
	(cd ${.CURDIR} && $(M4) $< > obj/$@)
	$(CHMOD) $(ROMODE) $@

ALL=	cs-hidden.cf cs-exposed.cf \
	sunos3.5-cs-exposed.cf sunos3.5-cs-hidden.cf \
	sunos4.1-cs-exposed.cf sunos4.1-cs-hidden.cf \
	ultrix4.1-cs-exposed.cf ultrix4.1-cs-hidden.cf \
	mail.cs.cf mail.eecs.cf ucbvax.cf vangogh.cf \
	chez.cf knecht.cf cogsci.cf

all: $(ALL)

clean cleandir:
	$(RM) $(ALL) core

depend install: 

# this is overkill, but....
M4FILES=../domain/cs.exposed.m4 \
	../domain/cs.hidden.m4 \
	../domain/Berkeley.m4 \
	../feature/no_wildcard_MX.m4 \
	../feature/use_cw_file.m4 \
	../hack/cssubdomain.m4 \
	../m4/boilerplate.m4 \
	../m4/cf.m4 \
	../m4/proto.m4 \
	../m4/version.m4 \
	../mailer/local.m4 \
	../mailer/smtp.m4 \
	../mailer/uucp.m4 \
	../ostype/bsd4.3.m4 \
	../ostype/bsd4.4.m4 \
	../ostype/riscos4.5.m4 \
	../ostype/sunos3.5.m4 \
	../ostype/sunos4.1.m4 \
	../ostype/ultrix4.1.m4 \

$(ALL):  $(M4FILES)

.include <bsd.prog.mk>
