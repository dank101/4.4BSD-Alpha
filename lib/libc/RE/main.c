#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

char *progname;
char *eprint();
int debug = 0;
int inline = 0;
int status = 0;

int copts = REG_EXTENDED;
int eopts = 0;
regoff_t startoff = 0;
regoff_t endoff = 0;

main(argc, argv)
int argc;
char *argv[];
{
	regex_t re;
#	define	NS	10
	regmatch_t subs[NS];
	char erbuf[100];
	int err;
	size_t len;
	int c;
	int errflg = 0;
	register int i;
	extern int optind;
	extern char *optarg;

	progname = argv[0];

	while ((c = getopt(argc, argv, "c:e:S:E:x")) != EOF)
		switch (c) {
		case 'c':	/* compile options */
			copts = options('c', optarg);
			break;
		case 'e':	/* execute options */
			eopts = options('e', optarg);
			break;
		case 'S':	/* start offset */
			startoff = (regoff_t)atoi(optarg);
			break;
		case 'E':	/* end offset */
			endoff = (regoff_t)atoi(optarg);
			break;
		case 'x':	/* Debugging. */
			debug++;
			break;
		case '?':
		default:
			errflg++;
			break;
		}
	if (errflg) {
		fprintf(stderr, "usage: %s ", progname);
		fprintf(stderr, "[-c copt][-C][-d] [re]\n");
		exit(2);
	}

	if (optind >= argc) {
		regress(stdin);
		exit(status);
	}

	err = regcomp(&re, argv[optind++], copts);
	if (err) {
		len = regerror(err, &re, erbuf, sizeof(erbuf));
		fprintf(stderr, "error REG_%s, %d/%d `%s'\n",
			eprint(err), len, sizeof(erbuf), erbuf);
		exit(status);
	}
	regprint(&re, stdout);	

	if (optind >= argc) {
		regfree(&re);
		exit(status);
	}

	if (eopts&REG_STARTEND) {
		subs[0].rm_so = startoff;
		subs[0].rm_eo = strlen(argv[optind]) - endoff;
	}
	err = regexec(&re, argv[optind], (size_t)NS, subs, eopts);
	if (err) {
		len = regerror(err, &re, erbuf, sizeof(erbuf));
		fprintf(stderr, "error REG_%s, %d/%d `%s'\n",
			eprint(err), len, sizeof(erbuf), erbuf);
		exit(status);
	}
	if (!(copts&REG_NOSUB)) {
		if (subs[0].rm_so != -1)
			printf("match `%.*s'\n",
					(int)(subs[0].rm_eo - subs[0].rm_so),
					argv[optind] + subs[0].rm_so);
		for (i = 1; i < NS; i++)
			if (subs[i].rm_so != -1)
				printf("(%d) `%.*s'\n", i,
					(int)(subs[i].rm_eo - subs[i].rm_so),
					argv[optind] + subs[i].rm_so);
	}
	exit(status);
}

/*
 - regress - main loop of regression test
 */
regress(in)
FILE *in;
{
	char inbuf[1000];
#	define	MAXF	10
	char *f[MAXF];
	int nf;
	int i;
	char erbuf[100];
	size_t ne;
	char *badpat = "invalid regular expression";
#	define	SHORT	10

	while (fgets(inbuf, sizeof(inbuf), in) != NULL) {
		inline++;
		if (inbuf[0] == '#' || inbuf[0] == '\n')
			continue;			/* NOTE CONTINUE */
		inbuf[strlen(inbuf)-1] = '\0';	/* get rid of stupid \n */
		if (debug)
			fprintf(stdout, "%d:\n", inline);
		nf = split(inbuf, f, MAXF, "\t\t");
		if (nf < 3) {
			fprintf(stderr, "bad input, line %d\n", inline);
			exit(1);
		}
		for (i = 0; i < nf; i++)
			if (strcmp(f[i], "\"\"") == 0)
				f[i] = "";
		if (nf <= 3)
			f[3] = NULL;
		if (nf <= 4)
			f[4] = NULL;
		try(f[0], f[1], f[2], f[3], f[4], options('c', f[1]));
		if (opt('&', f[1]))	/* try with either type of RE */
			try(f[0], f[1], f[2], f[3], f[4],
					options('c', f[1]) &~ REG_EXTENDED);
	}

	ne = regerror(REG_BADPAT, (regex_t *)NULL, erbuf, sizeof(erbuf));
	if (strcmp(erbuf, badpat) != 0 || ne != strlen(badpat)+1) {
		fprintf(stderr, "end: regerror() test gave `%s' not `%s'\n",
							erbuf, badpat);
		status = 1;
	}
	ne = regerror(REG_BADPAT, (regex_t *)NULL, erbuf, (size_t)SHORT);
	if (strncmp(erbuf, badpat, SHORT-1) != 0 || erbuf[SHORT-1] != '\0' ||
						ne != strlen(badpat)+1) {
		fprintf(stderr, "end: regerror() short test gave `%s' not `%.*s'\n",
						erbuf, SHORT-1, badpat);
		status = 1;
	}
}

/*
 - try - try and report on problems
 */
try(f0, f1, f2, f3, f4, opts)
char *f0;
char *f1;
char *f2;
char *f3;
char *f4;
int opts;			/* may not match f1 */
{
	regex_t re;
#	define	NSUBS	10
	regmatch_t subs[NSUBS];
	char *should[NSUBS*2];
	int nshould;
	char erbuf[100];
	int err;
	int len;
	char *type = (opts & REG_EXTENDED) ? "ERE" : "BRE";
	register int i;

	newline(f0);
	err = regcomp(&re, f0, opts);
	if (err != 0 && (!opt('C', f1) || err != efind(f2))) {
		/* unexpected error or wrong error */
		len = regerror(err, &re, erbuf, sizeof(erbuf));
		fprintf(stderr, "%d: %s error REG_%s, %d/%d `%s'\n",
					inline, type, eprint(err), len,
					sizeof(erbuf), erbuf);
		status = 1;
	} else if (err == 0 && opt('C', f1)) {
		/* unexpected success */
		fprintf(stderr, "%d: %s should have given REG_%s\n",
						inline, type, f2);
		status = 1;
		err = 1;	/* so we won't try regexec */
	}

	if (err != 0) {
		regfree(&re);
		return;
	}

	newline(f2);
	newline(f3);
	newline(f4);

	if (options('e', f1)&REG_STARTEND) {
		if (strchr(f2, '(') == NULL || strchr(f2, ')') == NULL)
			fprintf(stderr, "%d: bad STARTEND syntax\n", inline);
		subs[0].rm_so = strchr(f2, '(') - f2 + 1;
		subs[0].rm_eo = strchr(f2, ')') - f2;
	}
	err = regexec(&re, f2, NSUBS, subs, options('e', f1));
	if (err != 0 && (f3 != NULL || err != REG_NOMATCH)) {
		/* unexpected error or wrong error */
		len = regerror(err, &re, erbuf, sizeof(erbuf));
		fprintf(stderr, "%d: %s exec error REG_%s, %d/%d `%s'\n",
					inline, type, eprint(err), len,
					sizeof(erbuf), erbuf);
		status = 1;
	} else if (err == 0 && f3 == NULL) {
		/* unexpected success */
		fprintf(stderr, "%d: %s exec should have failed\n",
						inline, type);
		status = 1;
		err = 1;		/* just on principle */
	} else if (err == 0 && !(opts&REG_NOSUB) &&
			( subs[0].rm_so == -1 || subs[0].rm_eo == -1 )) {
		/* no report on what matched */
		fprintf(stderr, "%d: %s match not reported\n", inline, type);
		status = 1;
		err = 1;
	} else if (err == 0 && !(opts&REG_NOSUB) &&
			( subs[0].rm_eo - subs[0].rm_so != strlen(f3) ||
			strncmp(f2+subs[0].rm_so, f3, strlen(f3)) != 0 )) {
		/* matched wrong thing */
		fprintf(stderr, "%d: %s matched `%.*s' instead\n", inline,
				type, (int)(subs[0].rm_eo-subs[0].rm_so),
				f2+subs[0].rm_so);
		status = 1;
		err = 1;
	}

	if (err != 0 || f4 == NULL) {
		regfree(&re);
		return;
	}

	nshould = split(f4, should+1, NSUBS*2-1, ",");
	for (i = 1; i < NSUBS; i++) {
		if (i <= nshould && strcmp(should[i], "\"\"") == 0)
			should[i] = "";
		if (i > nshould && (subs[i].rm_so >= 0 || subs[i].rm_eo >= 0)) {
			fprintf(stderr, "%d: %s $%d was `%.*s'\n", inline,
				type, i, (int)(subs[i].rm_eo-subs[i].rm_so),
				f2+subs[i].rm_so);
			status = 1;
			err = 1;
		} else if (i > nshould) {
			/* okay */
		} else if (strcmp(should[i], "-") == 0 &&
				(subs[i].rm_so >= 0 || subs[i].rm_eo >= 0)) {
			fprintf(stderr, "%d: %s $%d was `%.*s'\n", inline,
				type, i, (int)(subs[i].rm_eo-subs[i].rm_so),
				f2+subs[i].rm_so);
			status = 1;
			err = 1;
		} else if (strcmp(should[i], "-") == 0) {
			/* okay */
		} else if (subs[i].rm_so < 0 || subs[i].rm_eo < 0) {
			fprintf(stderr, "%d: %s $%d didn't match\n", inline,
				type, i);
			status = 1;
			err = 1;
		} else if (subs[i].rm_eo - subs[i].rm_so != strlen(should[i]) ||
			strncmp(f2+subs[i].rm_so, should[i], strlen(should[i])) != 0) {
			fprintf(stderr, "%d: %s $%d matched `%.*s' instead\n",
				inline, type, i,
				(int)(subs[i].rm_eo-subs[i].rm_so),
				f2+subs[i].rm_so);
		}
	}

	regfree(&re);
}

/*
 - options - pick options out of a regression-test string
 */
int
options(type, s)
char type;			/* 'c' compile, 'e' exec */
char *s;
{
	register char *p;
	register int o = (type == 'c') ? copts : eopts;
	register char *legal = (type == 'c') ? "bisn" : "^$#tl";

	for (p = s; *p != '\0'; p++)
		if (strchr(legal, *p) != NULL)
			switch (*p) {
			case 'b':
				o &= ~REG_EXTENDED;
				break;
			case 'i':
				o |= REG_ICASE;
				break;
			case 's':
				o |= REG_NOSUB;
				break;
			case 'n':
				o |= REG_NEWLINE;
				break;
			case '^':
				o |= REG_NOTBOL;
				break;
			case '$':
				o |= REG_NOTEOL;
				break;
			case '#':
				o |= REG_STARTEND;
				break;
			case 't':	/* trace */
				o |= REG_TRACE;
				break;
			case 'l':	/* force long representation */
				o |= REG_LARGE;
				break;
			}
	return(o);
}

/*
 - opt - is a particular option in a regression string?
 */
int				/* predicate */
opt(c, s)
char c;
char *s;
{
	return(strchr(s, c) != NULL);
}

/*
 - newline - turn N into \n
 */
newline(p)
register char *p;
{
	if (p == NULL)
		return;

	for (; *p != '\0'; p++)
		if (*p == 'N')
			*p = '\n';
}
