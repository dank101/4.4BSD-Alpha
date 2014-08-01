/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char sccsid[] = "@(#)print.c	5.13 (Berkeley) 7/9/92";
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <math.h>
#include <nlist.h>
#include <tzfile.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "ps.h"

#ifdef SPPWAIT
#define NEWVM
#endif

#ifdef NEWVM
#include <vm/vm.h>
#include <sys/ucred.h>
#include <sys/kinfo_proc.h>
#else
#include <machine/pte.h>
#include <sys/vmparam.h>
#include <sys/vm.h>
#endif

static void printval __P((char*, struct var *));

void
printheader()
{
	register VAR *v;
	register struct varent *vent;

	for (vent = vhead; vent; vent = vent->next) {
		v = vent->var;
		if (v->flag & LJUST) {
			if (vent->next == NULL)	/* last one */
				(void)printf("%s", v->header);
			else
				(void)printf("%-*s", v->width, v->header);
		} else
			(void)printf("%*s", v->width, v->header);
		if (vent->next != NULL)
			(void)putchar(' ');
	}
	(void)putchar('\n');
}

void
command(k, ve)
	KINFO *k;
	VARENT *ve;
{
	extern int termwidth, totwidth;
	VAR *v;

	v = ve->var;
	if (ve->next == NULL) {
		/* last field */
		if (termwidth == UNLIMITED) {
			if (k->ki_env)
				(void)printf("%s ", k->ki_env);
			(void)printf("%s", k->ki_args);
		} else {
			register int left = termwidth - (totwidth - v->width);
			register char *cp;

			if (left < 1) /* already wrapped, just use std width */
				left = v->width;
			cp = k->ki_env;
			if (cp != 0) {
				while (--left >= 0 && *cp)
					(void)putchar(*cp++);
				if (--left >= 0)
					putchar(' ');
			}
			cp = k->ki_args;
			while (--left >= 0 && *cp)
				(void)putchar(*cp++);
		}
	} else
		/* XXX env? */
		(void)printf("%-*.*s", v->width, v->width, k->ki_args);
}

void
ucomm(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	(void)printf("%-*s", v->width, KI_PROC(k)->p_comm);
}

void
logname(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
#ifndef NEWVM
	(void)printf("%-*s", v->width, KI_PROC(k)->p_logname);
#else /* NEWVM */
	(void)printf("%-*s", v->width, KI_EPROC(k)->e_login);
#endif /* NEWVM */
}

void
state(k, ve)
	KINFO *k;
	VARENT *ve;
{
	register struct proc *p;
	register int flag;
	register char *cp;
	VAR *v;
	char buf[16];

	v = ve->var;
	p = KI_PROC(k);
	flag = p->p_flag;
	cp = buf;

	switch (p->p_stat) {

	case SSTOP:
		*cp = 'T';
		break;

	case SSLEEP:
		if (flag & SSINTR)	/* interuptable (long) */
			*cp = p->p_slptime >= MAXSLP ? 'I' : 'S';
		else
			*cp = 'D';
		break;

	case SRUN:
	case SIDL:
		*cp = 'R';
		break;

	case SZOMB:
		*cp = 'Z';
		break;

	default:
		*cp = '?';
	}
	cp++;
	if (flag & SLOAD) {
#ifndef NEWVM
		if (p->p_rssize > p->p_maxrss)
			*cp++ = '>';
#endif
	} else
		*cp++ = 'W';
	if (p->p_nice < NZERO)
		*cp++ = '<';
	else if (p->p_nice > NZERO)
		*cp++ = 'N';
#ifndef NEWVM
	if (flag & SUANOM)
		*cp++ = 'A';
	else if (flag & SSEQL)
		*cp++ = 'S';
#endif
	if (flag & STRC)
		*cp++ = 'X';
	if (flag & SWEXIT && p->p_stat != SZOMB)
		*cp++ = 'E';
#ifdef NEWVM
	if (flag & SPPWAIT)
#else
	if (flag & SVFORK)
#endif
		*cp++ = 'V';
#ifdef NEWVM
	if (flag & (SSYS|SLOCK|SKEEP|SPHYSIO))
#else
	if (flag & (SSYS|SLOCK|SULOCK|SKEEP|SPHYSIO))
#endif
		*cp++ = 'L';
	if (KI_EPROC(k)->e_flag & EPROC_SLEADER)
		*cp++ = 's';
	if ((flag & SCTTY) && KI_EPROC(k)->e_pgid == KI_EPROC(k)->e_tpgid)
		*cp++ = '+';
	*cp = '\0';
	(void)printf("%-*s", v->width, buf);
}

void
pri(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	(void)printf("%*d", v->width, KI_PROC(k)->p_pri - PZERO);
}

void
uname(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
#ifndef NEWVM
	(void)printf("%-*s", v->width, user_from_uid(KI_PROC(k)->p_uid, 0));
#else /* NEWVM */
	(void)printf("%-*s", v->width,
		user_from_uid(KI_EPROC(k)->e_ucred.cr_uid, 0));
#endif /* NEWVM */
}

void
runame(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
#ifndef NEWVM
	(void)printf("%-*s", v->width, user_from_uid(KI_PROC(k)->p_ruid, 0));
#else /* NEWVM */
	(void)printf("%-*s", v->width,
		user_from_uid(KI_EPROC(k)->e_pcred.p_ruid, 0));
#endif /* NEWVM */
}

void
tdev(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;
	dev_t dev;
	char buff[16];

	v = ve->var;
	dev = KI_EPROC(k)->e_tdev;
	if (dev == NODEV)
		(void)printf("%*s", v->width, "??");
	else {
		(void)snprintf(buff, sizeof(buff),
		    "%d/%d", major(dev), minor(dev));
		(void)printf("%*s", v->width, buff);
	}
}

void
tname(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;
	dev_t dev;
	char *ttname;

	v = ve->var;
	dev = KI_EPROC(k)->e_tdev;
	if (dev == NODEV || (ttname = devname(dev, S_IFCHR)) == NULL)
		(void)printf("%-*s", v->width, "??");
	else {
		if (strncmp(ttname, "tty", 3) == 0)
			ttname += 3;
		(void)printf("%*.*s%c", v->width-1, v->width-1, ttname,
			KI_EPROC(k)->e_flag & EPROC_CTTY ? ' ' : '-');
	}
}

void
longtname(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;
	dev_t dev;
	char *ttname;

	v = ve->var;
	dev = KI_EPROC(k)->e_tdev;
	if (dev == NODEV || (ttname = devname(dev, S_IFCHR)) == NULL)
		(void)printf("%-*s", v->width, "??");
	else
		(void)printf("%-*s", v->width, ttname);
}

void
started(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;
	static time_t now;
	struct tm *tp;
	char buf[100];

	v = ve->var;
	if (!k->ki_u.u_valid) {
		(void)printf("%-*s", v->width, "-");
		return;
	}

	tp = localtime(&k->ki_u.u_start.tv_sec);
	if (!now)
		(void)time(&now);
	if (now - k->ki_u.u_start.tv_sec < 24 * SECSPERHOUR) {
		static char fmt[] = "%l:@M%p";
		fmt[3] = '%';			/* I *hate* SCCS... */
		(void)strftime(buf, sizeof(buf) - 1, fmt, tp);
	} else if (now - k->ki_u.u_start.tv_sec < 7 * SECSPERDAY) {
		static char fmt[] = "%a@I%p";
		fmt[2] = '%';			/* I *hate* SCCS... */
		(void)strftime(buf, sizeof(buf) - 1, fmt, tp);
	} else
		(void)strftime(buf, sizeof(buf) - 1, "%e%b%y", tp);
	(void)printf("%-*s", v->width, buf);
}

void
lstarted(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;
	char buf[100];

	v = ve->var;
	if (!k->ki_u.u_valid) {
		(void)printf("%-*s", v->width, "-");
		return;
	}
	(void)strftime(buf, sizeof(buf) -1, "%C",
	    localtime(&k->ki_u.u_start.tv_sec));
	(void)printf("%-*s", v->width, buf);
}

void
wchan(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	if (KI_PROC(k)->p_wchan) {
		if (KI_PROC(k)->p_wmesg)
			(void)printf("%-*.*s", v->width, v->width, 
				      KI_EPROC(k)->e_wmesg);
		else
			(void)printf("%-*x", v->width,
			    (int)KI_PROC(k)->p_wchan &~ KERNBASE);
	} else
		(void)printf("%-*s", v->width, "-");
}

#define pgtok(a)        (((a)*NBPG)/1024)

void
vsize(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	(void)printf("%*d", v->width,
#ifndef NEWVM
	    pgtok(KI_PROC(k)->p_dsize + KI_PROC(k)->p_ssize + KI_EPROC(k)->e_xsize));
#else /* NEWVM */
	    pgtok(KI_EPROC(k)->e_vm.vm_dsize + KI_EPROC(k)->e_vm.vm_ssize +
		KI_EPROC(k)->e_vm.vm_tsize));
#endif /* NEWVM */
}

void
rssize(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
#ifndef NEWVM
	(void)printf("%*d", v->width,
	    pgtok(KI_PROC(k)->p_rssize + (KI_EPROC(k)->e_xccount ?
	    (KI_EPROC(k)->e_xrssize / KI_EPROC(k)->e_xccount) : 0)));
#else /* NEWVM */
	/* XXX don't have info about shared */
	(void)printf("%*d", v->width, pgtok(KI_EPROC(k)->e_vm.vm_rssize));
#endif /* NEWVM */
}

void
p_rssize(k, ve)		/* doesn't account for text */
	KINFO *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
#ifndef NEWVM
	(void)printf("%*d", v->width, pgtok(KI_PROC(k)->p_rssize));
#else /* NEWVM */
	(void)printf("%*d", v->width, pgtok(KI_EPROC(k)->e_vm.vm_rssize));
#endif /* NEWVM */
}

void
cputime(k, ve)
	KINFO *k;
	VARENT *ve;
{
	extern int sumrusage;
	VAR *v;
	long secs;
	long psecs;	/* "parts" of a second. first micro, then centi */
	char obuff[128];

	v = ve->var;
	if (KI_PROC(k)->p_stat == SZOMB || !k->ki_u.u_valid) {
		secs = 0;
		psecs = 0;
	} else {
		/*
		 * This counts time spent handling interrupts.  We could
		 * fix this, but it is not 100% trivial (and interrupt
		 * time fractions only work on the sparc anyway).	XXX
		 */
		secs = KI_PROC(k)->p_rtime.tv_sec;
		psecs = KI_PROC(k)->p_rtime.tv_usec;
		if (sumrusage) {
			secs += k->ki_u.u_cru.ru_utime.tv_sec +
				k->ki_u.u_cru.ru_stime.tv_sec;
			psecs += k->ki_u.u_cru.ru_utime.tv_usec +
				k->ki_u.u_cru.ru_stime.tv_usec;
		}
		/*
		 * round and scale to 100's
		 */
		psecs = (psecs + 5000) / 10000;
		secs += psecs / 100;
		psecs = psecs % 100;
	}
	(void)snprintf(obuff, sizeof(obuff),
	    "%3ld:%02ld.%02ld", secs/60, secs%60, psecs);
	(void)printf("%*s", v->width, obuff);
}

double
getpcpu(k)
	KINFO *k;
{
	extern fixpt_t ccpu;
	extern int fscale, nlistread, rawcpu;
	struct proc *p;
	static int failure;

	if (!nlistread)
		failure = donlist();
	if (failure)
		return (0.0);

	p = KI_PROC(k);
#define	fxtofl(fixpt)	((double)(fixpt) / fscale)

	/* XXX - I don't like this */
	if (p->p_time == 0 || (p->p_flag & SLOAD) == 0)
		return (0.0);
	if (rawcpu)
		return (100.0 * fxtofl(p->p_pctcpu));
	return (100.0 * fxtofl(p->p_pctcpu) /
		(1.0 - exp(p->p_time * log(fxtofl(ccpu)))));
}

void
pcpu(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	(void)printf("%*.1f", v->width, getpcpu(k));
}

double
getpmem(k)
	KINFO *k;
{
	extern int mempages, nlistread;
	static int failure;
	struct proc *p;
	struct eproc *e;
	double fracmem;
	int szptudot;

	if (!nlistread)
		failure = donlist();
	if (failure)
		return (0.0);

	p = KI_PROC(k);
	e = KI_EPROC(k);
	if ((p->p_flag & SLOAD) == 0)
		return (0.0);
#ifndef NEWVM
	szptudot = UPAGES + clrnd(ctopt(p->p_dsize + p->p_ssize + e->e_xsize));
	fracmem = ((float)p->p_rssize + szptudot)/CLSIZE/mempages;
	if (p->p_textp && e->e_xccount)
		fracmem += ((float)e->e_xrssize)/CLSIZE/e->e_xccount/mempages;
#else /* NEWVM */
	/* XXX want pmap ptpages, segtab, etc. (per architecture) */
	szptudot = UPAGES;
	/* XXX don't have info about shared */
	fracmem = ((float)e->e_vm.vm_rssize + szptudot)/CLSIZE/mempages;
#endif /* NEWVM */
	return (100.0 * fracmem);
}

void
pmem(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	(void)printf("%*.1f", v->width, getpmem(k));
}

void
pagein(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	(void)printf("%*d", v->width, 
	    k->ki_u.u_valid ? k->ki_u.u_ru.ru_majflt : 0);
}

void
maxrss(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
#ifndef NEWVM	/* not yet */
	if (KI_PROC(k)->p_maxrss != (RLIM_INFINITY/NBPG))
		(void)printf("%*d", v->width, pgtok(KI_PROC(k)->p_maxrss));
	else
#endif /* NEWVM */
		(void)printf("%*s", v->width, "-");
}

void
tsize(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
#ifndef NEWVM
	(void)printf("%*d", v->width, pgtok(KI_EPROC(k)->e_xsize));
#else /* NEWVM */
	(void)printf("%*d", v->width, pgtok(KI_EPROC(k)->e_vm.vm_tsize));
#endif /* NEWVM */
}

#ifndef NEWVM
void
trss(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	(void)printf("%*d", v->width, pgtok(KI_EPROC(k)->e_xrssize));
}
#endif /* NEWVM */

/*
 * Generic output routines.  Print fields from various prototype
 * structures.
 */
void
pvar(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	printval((char *)((char *)KI_PROC(k) + v->off), v);
}

void
evar(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	printval((char *)((char *)KI_EPROC(k) + v->off), v);
}

void
uvar(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	if (k->ki_u.u_valid)
		printval((char *)((char *)&k->ki_u + v->off), v);
	else
		(void)printf("%*s", v->width, "-");
}

void
rvar(k, ve)
	KINFO *k;
	VARENT *ve;
{
	VAR *v;

	v = ve->var;
	if (k->ki_u.u_valid)
		printval((char *)((char *)(&k->ki_u.u_ru) + v->off), v);
	else
		(void)printf("%*s", v->width, "-");
}

static void
printval(bp, v)
	char *bp;
	VAR *v;
{
	static char ofmt[32] = "%";
	register char *fcp, *cp;

	cp = ofmt + 1;
	fcp = v->fmt;
	if (v->flag & LJUST)
		*cp++ = '-';
	*cp++ = '*';
	while (*cp++ = *fcp++);

	switch (v->type) {
	case CHAR:
		(void)printf(ofmt, v->width, *(char *)bp);
		break;
	case UCHAR:
		(void)printf(ofmt, v->width, *(u_char *)bp);
		break;
	case SHORT:
		(void)printf(ofmt, v->width, *(short *)bp);
		break;
	case USHORT:
		(void)printf(ofmt, v->width, *(u_short *)bp);
		break;
	case LONG:
		(void)printf(ofmt, v->width, *(long *)bp);
		break;
	case ULONG:
		(void)printf(ofmt, v->width, *(u_long *)bp);
		break;
	case KPTR:
		(void)printf(ofmt, v->width, *(u_long *)bp &~ KERNBASE);
		break;
	default:
		err("unknown type %d", v->type);
	}
}
