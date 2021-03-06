/*-
 * Copyright (c) 1983 The Regents of the University of California.
 * All rights reserved.
 *
 * This module is believed to contain source code proprietary to AT&T.
 * Use and redistribution is subject to the Berkeley Software License
 * Agreement and your Software Agreement with AT&T (Western Electric).
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)apply.c	5.4 (Berkeley) 4/17/91";
#endif /* not lint */

/*
 * apply - apply a command to a set of arguments
 *
 *	apply echo * == ls
 *	apply -2 cmp A1 B1 A2 B2   compares A's with B's
 *	apply "ln %1 /usr/fred/dir" *  duplicates a directory
 */
#include <paths.h>
#include <stdio.h>

char	*cmdp;
#define	NCHARS 512
char	cmd[512];
char	defargs=1;
#define	DEFARGCHAR	'%'
char	argchar=DEFARGCHAR;
int	nchars;
extern	char *getenv();

main(argc, argv)
	char *argv[];
{
	register n;
	while(argc>2 && argv[1][0]=='-'){
		if(argv[1][1]=='a'){
			argchar=argv[1][2];
			if(argchar=='\0')
				argchar=DEFARGCHAR;
		} else {
			defargs = atoi(&argv[1][1]);
			if(defargs < 0)
				defargs = 1;
		}
		--argc; ++argv;
	}
	if(argc<2){
		fprintf(stderr, "usage: apply [-#] [-ac] cmd arglist\n");
		exit(1);
	}
	argc -= 2;
	cmdp = argv[1];
	argv += 2;
	while(n=docmd(argc, argv)){
		argc -= n;
		argv += n;
	}
}
char
addc(c)
	char c;
{
	if(nchars++>=NCHARS){
		fprintf(stderr, "apply: command too long\n");
		exit(1);
	}
	return(c);
}
char *
addarg(s, t)
	register char *s, *t;
{
	while(*t = addc(*s++))
		*t++;
	return(t);
}
docmd(argc, argv)
	char *argv[];
{
	register char *p, *q;
	register max, i;
	char gotit;
	if(argc<=0)
		return(0);
	nchars = 0;
	max = 0;
	gotit = 0;
	p = cmdp;
	q = cmd;
	while(*q = addc(*p++)){
		if(*q++!=argchar || *p<'1' || '9'<*p)
			continue;
		if((i= *p++-'1') > max)
			max = i;
		if(i>=argc){
	Toofew:
			fprintf(stderr, "apply: expecting argument(s) after `%s'\n", argv[argc-1]);
			exit(1);
		}
		q = addarg(argv[i], q-1);
		gotit++;
	}
	if(defargs!=0 && gotit==0){
		if(defargs>argc)
			goto Toofew;
		for(i=0; i<defargs; i++){
			*q++ = addc(' ');
			q = addarg(argv[i], q);
		}
	}
	i = system(cmd);
	if(i == 127){
		fprintf(stderr, "apply: no shell!\n");
		exit(1);
	}
	return(max==0? (defargs==0? 1 : defargs) : max+1);
}
system(s)
char *s;
{
	int status, pid, w;
	char *shell = getenv("SHELL");

	if ((pid = fork()) == 0) {
		execl(shell ? shell : _PATH_BSHELL, "sh", "-c", s, 0);
		_exit(127);
	}
	if(pid == -1){
		fprintf(stderr, "apply: can't fork\n");
		exit(1);
	}
	while ((w = wait(&status)) != pid && w != -1)
		;
	if (w == -1)
		status = -1;
	return(status);
}
