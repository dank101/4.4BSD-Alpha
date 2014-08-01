# -DNDEBUG -O
CFLAGS = -I. -Dconst= -DPOSIX_MISTAKE
LINTFLAGS = -I. -Dstatic= -Dconst=
LDFLAGS =
OBJMISSING = split.o memmove.o
SRCMISSING = split.c memmove.c
OBJS = regcomp.o regexec.o regerror.o regfree.o debug.o main.o $(OBJMISSING)
SRCS = regcomp.c regexec.c regerror.c regfree.c debug.c main.c $(SRCMISSING)
JUNKLINT =possible pointer alignment|null effect

it:	rs

re:	$(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

regcomp.o regexec.o regfree.o debug.o:	utils.h regex.h regex2.h
regexec.o:	engine.c
regerror.o:	utils.h
regcomp.o:	cclass.h cname.h

r:	re tests
	re <tests

rs:	re tests
	re <tests
	re -el <tests

rd:	re tests
	re -d <tests

t:	re tests
	-time re <tests
	-time re -cs <tests
	-time re -el <tests
	-time re -cs -el <tests

e:	echeck re tests
	re -d <tests | awk -f echeck

lint:	$(SRCS)
	lint $(LINTFLAGS) -hpa $(SRCS) 2>&1 | egrep -v '$(JUNKLINT)' | tee $@

print:
	ti *.h todo | hplist
	hplist *.c

clean:
	rm -f *.o re core

tidy:
	rm -f junk* core

dtr:	README Makefile regex.3 regex.7 tests *.h *.c
	makedtr README Makefile regex.3 regex.7 tests *.h *.c >$@
