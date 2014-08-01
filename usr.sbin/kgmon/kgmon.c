/*
 * Copyright (c) 1983, 1992 The Regents of the University of California.
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
char copyright[] =
"@(#) Copyright (c) 1983, 1992 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)kgmon.c	5.14 (Berkeley) 7/10/92";
#endif /* not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/gmon.h>
#include <errno.h>
#include <kvm.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nlist.h>
#include <ctype.h>
#include <paths.h>

struct nlist nl[] = {
#define	N_GMONPARAM	0
	{ "__gmonparam" },
#define	N_KCOUNT	1
	{ "_kcount" },
#define	N_PROFHZ	2
	{ "_profhz" },
	0,
};

/*
 * We should call this _gmonparam for consistency, but that would cause a 
 * problem if we want to profile this program itself.
 */
struct gmonparam gmonparam;

u_short *kcount;
int profhz;

kvm_t	*kd;
int	bflag, hflag, kflag, rflag, pflag;
int	debug = 0;

#define KREAD(kd, addr, s)\
	kvm_read(kd, addr, (void *)(s), sizeof*(s)) != (sizeof*(s))

/*
 * Build the gmon header and write it to a file.
 */
void
dumphdr(FILE *fp, struct gmonparam *p, int ksize)
{
	struct gmonhdr h;

	/* zero out the unused fields */
	bzero(&h, sizeof(h));

	h.lpc = p->lowpc;
	h.hpc = p->highpc;
	h.ncnt = ksize + sizeof(h);
	h.version = GMONVERSION;
	h.profrate = profhz;

	fwrite((char *)&h, sizeof(h), 1, fp);
}

/*
 * Dump a range of kernel memory to a file.
 */
void
dumpbuf(FILE *fp, u_long addr, int cc)
{
	int ret, n;
	char buf[8192];

	while (cc > 0) {
		n = MIN(cc, sizeof(buf));
		if ((ret = kvm_read(kd, addr, buf, n)) != n) {
			(void)fprintf(stderr,
			    "kgmon: read kmem: read %d, got %d: %s\n",
			    n, ret, kvm_geterr(kd));
			exit(4);
		}
		if ((ret = fwrite(buf, n, 1, fp)) != 1) {
			perror("kgmon: gmon.out");
			exit(1);
		}
		addr += n;
		cc -= n;
	}
}

/*
 * Enable or disable kernel profiling according to the state variable.
 */
void
setprof(int state)
{
	struct gmonparam *p = (struct gmonparam *)nl[N_GMONPARAM].n_value;

	if (kvm_write(kd, (u_long)&p->state, (void *)&state, sizeof(state)) !=
	    sizeof(state))
		(void)fprintf(stderr,
		    "kgmon: warning: can't turn profiling %s\n",
		    state == GMON_PROF_OFF ? "off" : "on");
}

/*
 * Build the gmon.out file.
 */
void
dumpstate(struct gmonparam *p)
{
	register FILE *fp;
	struct rawarc rawarc;
	struct tostruct *tos;
	u_long frompc, addr;
	u_short *froms;
	int i, n;
	int fromindex, endfrom, toindex;
	u_int fromssize, tossize, ksize;

	setprof(GMON_PROF_OFF);
	fp = fopen("gmon.out", "w");
	if (fp == 0) {
		perror("gmon.out");
		return;
	}
	ksize = p->textsize / HISTFRACTION;
	dumphdr(fp, p, ksize);
	dumpbuf(fp, (u_long)kcount, ksize);

	fromssize = p->textsize / HASHFRACTION;
	froms = (u_short *)malloc(fromssize);
	i = kvm_read(kd, (u_long)p->froms, (void *)froms, fromssize);
	if (i != fromssize) {
		(void)fprintf(stderr, "kgmon: read kmem: read %u, got %d: %s",
		    fromssize, i, strerror(errno));
		exit(5);
	}
	tossize = (p->textsize * ARCDENSITY / 100) * sizeof(struct tostruct);
	tos = (struct tostruct *)malloc(tossize);
	i = kvm_read(kd, (u_long)p->tos, (void *)tos, tossize);
	if (i != tossize) {
		(void)fprintf(stderr, "kgmon: read kmem: read %u, got %d: %s",
		    tossize, i, kvm_geterr(kd));
		exit(6);
	}
	if (debug)
		(void)fprintf(stderr, "lowpc 0x%x, textsize 0x%x\n",
			      p->lowpc, p->textsize);
	endfrom = fromssize / sizeof(*froms);
	for (fromindex = 0; fromindex < endfrom; ++fromindex) {
		if (froms[fromindex] == 0)
			continue;
		frompc = (u_long)p->lowpc +
		    (fromindex * HASHFRACTION * sizeof(*froms));
		for (toindex = froms[fromindex]; toindex != 0;
		   toindex = tos[toindex].link) {
			if (debug)
			    (void)fprintf(stderr,
			    "[mcleanup] frompc 0x%x selfpc 0x%x count %d\n" ,
			    frompc, tos[toindex].selfpc, tos[toindex].count);
			rawarc.raw_frompc = frompc;
			rawarc.raw_selfpc = (u_long)tos[toindex].selfpc;
			rawarc.raw_count = tos[toindex].count;
			fwrite((char *)&rawarc, sizeof(rawarc), 1, fp);
		}
	}
	fclose(fp);
}

/*
 * Zero out a region of kernel memory.
 */
int
kzero(u_long addr, int cc)
{
	static char zbuf[MAXBSIZE];

	while (cc > 0) {
		register int n = MIN(cc, sizeof(zbuf));

		if (kvm_write(kd, addr, zbuf, n) != n)
			return (-1);
		addr += n;
		cc -= n;
	}
	return (0);
}

/*
 * Reset the kernel profiling date structures.
 */
void
reset(struct gmonparam *p)
{
	int fromssize, tossize, ksize;

	setprof(GMON_PROF_OFF);

	ksize = p->textsize / HISTFRACTION;
	if (kzero((u_long)kcount, ksize)) {
		(void)fprintf(stderr, "kgmon: sbuf write: %s\n",
		    kvm_geterr(kd));
		exit(7);
	}
	fromssize = p->textsize / HASHFRACTION;
	if (kzero((u_long)p->froms, fromssize)) {
		(void)fprintf(stderr, "kgmon: kfroms write: %s\n",
		    kvm_geterr(kd));
		exit(8);
	}
	tossize = (p->textsize * ARCDENSITY / 100) * sizeof(struct tostruct);
	if (kzero((u_long)p->tos, tossize)) {
		(void)fprintf(stderr, "kgmon: ktos write: %s\n",
		    kvm_geterr(kd));
		exit(9);
	}
}

int
main(int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	int ch, mode, disp, openmode;
	char *system, *kmemf;
	char errbuf[_POSIX2_LINE_MAX];

	kmemf = NULL;
	system = NULL;
	while ((ch = getopt(argc, argv, "M:N:bhpr")) != EOF) {
		switch((char)ch) {

		case 'M':
			kmemf = optarg;
			kflag = 1;
			break;

		case 'N':
			system = optarg;
			break;

		case 'b':
			bflag = 1;
			break;

		case 'h':
			hflag = 1;
			break;

		case 'p':
			pflag = 1;
			break;

		case 'r':
			rflag = 1;
			break;

		default:
			(void)fprintf(stderr,
			    "usage: kgmon [-bhrp] [-M core] [-N system]\n");
			exit(1);
		}
	}
	argc -= optind;
	argv += optind;

#define BACKWARD_COMPATIBILITY
#ifdef	BACKWARD_COMPATIBILITY
	if (*argv) {
		system = *argv;
		if (*++argv) {
			kmemf = *argv;
			++kflag;
		}
	}
#endif
	if (system == NULL)
		system = _PATH_UNIX;
	openmode = (bflag || hflag || pflag || rflag) ? O_RDWR : O_RDONLY;
	kd = kvm_openfiles(system, kmemf, NULL, openmode, errbuf);
	if (kd == NULL) {
		if (openmode == O_RDWR) {
			openmode = O_RDONLY;
			kd = kvm_openfiles(system, kmemf, NULL, O_RDONLY,
			    errbuf);
		}
		if (kd == NULL) {
			(void)fprintf(stderr, "kgmon: kvm_openfiles: %s\n",
			    errbuf);
			exit(2);
		}
		(void)fprintf(stderr, "kgmon: kernel opened read-only\n");
		if (rflag)
			(void)fprintf(stderr, "-r supressed\n");
		if (bflag)
			(void)fprintf(stderr, "-b supressed\n");
		if (hflag)
			(void)fprintf(stderr, "-h supressed\n");
		rflag = bflag = hflag = 0;
	}
	if (kvm_nlist(kd, nl) < 0) {
		(void)fprintf(stderr, "kgmon: %s: no namelist\n", system);
		exit(2);
	}
	if (!nl[N_GMONPARAM].n_value) {
		(void)fprintf(stderr,
		    "kgmon: profiling not defined in kernel.\n");
		exit(10);
	}
	if (KREAD(kd, nl[N_GMONPARAM].n_value, &gmonparam))
		(void)fprintf(stderr,
		    "kgmon: read kmem: %s\n", kvm_geterr(kd));
	if (KREAD(kd, nl[N_KCOUNT].n_value, &kcount))
		(void)fprintf(stderr, "kgmon: read kmem: %s\n",
			      kvm_geterr(kd));
	if (KREAD(kd, nl[N_PROFHZ].n_value, &profhz))
		(void)fprintf(stderr, "kgmon: read kmem: %s\n",
			      kvm_geterr(kd));

	mode = gmonparam.state;
	if (hflag)
		disp = GMON_PROF_OFF;
	else if (bflag)
		disp = GMON_PROF_ON;
	else
		disp = mode;
	if (pflag) {
		if (openmode == O_RDONLY && mode == GMON_PROF_ON)
			(void)fprintf(stderr, "data may be inconsistent\n");
		dumpstate(&gmonparam);
	}
	if (rflag)
		reset(&gmonparam);
	if (openmode == O_RDWR)
		setprof(disp);
	(void)fprintf(stdout, "kernel profiling is %s.\n",
		      disp == GMON_PROF_OFF ? "off" : "running");

	return (0);
}
