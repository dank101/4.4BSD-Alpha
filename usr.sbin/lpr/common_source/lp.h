/*
 * Copyright (c) 1983 Regents of the University of California.
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
 *
 *	@(#)lp.h	5.6 (Berkeley) 7/21/92
 */

/*
 * Global definitions for the line printer system.
 */

extern int	DU;		/* daeomon user-id */
extern int	MX;		/* maximum number of blocks to copy */
extern int	MC;		/* maximum number of copies allowed */
extern char	*LP;		/* line printer device name */
extern char	*RM;		/* remote machine name */
extern char	*RG;		/* restricted group */
extern char	*RP;		/* remote printer name */
extern char	*LO;		/* lock file name */
extern char	*ST;		/* status file name */
extern char	*SD;		/* spool directory */
extern char	*AF;		/* accounting file */
extern char	*LF;		/* log file for error messages */
extern char	*OF;		/* name of output filter (created once) */
extern char	*IF;		/* name of input filter (created per job) */
extern char	*RF;		/* name of fortran text filter (per job) */
extern char	*TF;		/* name of troff(1) filter (per job) */
extern char	*NF;		/* name of ditroff(1) filter (per job) */
extern char	*DF;		/* name of tex filter (per job) */
extern char	*GF;		/* name of graph(1G) filter (per job) */
extern char	*VF;		/* name of raster filter (per job) */
extern char	*CF;		/* name of cifplot filter (per job) */
extern char	*FF;		/* form feed string */
extern char	*TR;		/* trailer string to be output when Q empties */
extern short	SC;		/* suppress multiple copies */
extern short	SF;		/* suppress FF on each print job */
extern short	SH;		/* suppress header page */
extern short	SB;		/* short banner instead of normal header */
extern short	HL;		/* print header last */
extern short	RW;		/* open LP for reading and writing */
extern short	PW;		/* page width */
extern short	PX;		/* page width in pixels */
extern short	PY;		/* page length in pixels */
extern short	PL;		/* page length */
extern short	BR;		/* baud rate if lp is a tty */
extern int	FC;		/* flags to clear if lp is a tty */
extern int	FS;		/* flags to set if lp is a tty */
extern int	XC;		/* flags to clear for local mode */
extern int	XS;		/* flags to set for local mode */
extern short	RS;		/* restricted to those with local accounts */

extern char	line[BUFSIZ];
extern char	pbuf[];		/* buffer for printcap entry */
extern char	*bp;		/* pointer into ebuf for pgetent() */
extern char	*name;		/* program name */
extern char	*printer;	/* printer name */
				/* host machine name */
extern char	host[MAXHOSTNAMELEN];
extern char	*from;		/* client's machine name */
extern int	sendtorem;	/* are we sending to a remote? */

/*
 * Structure used for building a sorted list of control files.
 */
struct queue {
	time_t	q_time;			/* modification time */
	char	q_name[MAXNAMLEN+1];	/* control file name */
};

#include <sys/cdefs.h>
struct dirent;

void     blankfill __P((int));
char	*checkremote __P((void));
int      chk __P((char *));
void     displayq __P((int));
void     dump __P((char *, char *, int));
void     endprent __P((void));
void	 fatal __P((const char *, ...));
int	 getline __P((FILE *));
int	 getport __P((char *));
int      getprent __P((char *));
int	 getq __P((struct queue *(*[])));
void     header __P((void));
void     inform __P((char *));
int      inlist __P((char *, char *));
int      iscf __P((struct dirent *));
int      isowner __P((char *, char *));
void     ldump __P((char *, char *, int));
int      lockchk __P((char *));
void     prank __P((int));
void     process __P((char *));
void     rmjob __P((void));
void     rmremote __P((void));
void     show __P((char *, char *, int));
int      startdaemon __P((char *));
int      pgetent __P((char *, char *));
int      pgetflag __P((char *));
int      pgetnum __P((char *));
char    *pgetstr __P((char *, char **));
int      pnamatch __P((char *));
int      pnchktc __P((void));
void     warn __P((void));
