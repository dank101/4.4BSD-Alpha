#	@(#)Makefile	7.1 (Berkeley) 7/13/92

sun_sysent.c sun_syscalls.c sun_syscall.h: makesyscalls.sh syscalls.master
	-mv -f sun_sysent.c sun_sysent.c.bak
	-mv -f sun_syscalls.c sun_syscalls.c.bak
	-mv -f sun_syscall.h sun_syscall.h.bak
	sh makesyscalls.sh syscalls.master
