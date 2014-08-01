#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <regex.h>

#include "utils.h"

static struct rerr {
	int code;
	char *name;
	char *explain;
} rerrs[] = {
	REG_NOMATCH,	"NOMATCH",	"regexec() failed to match",
	REG_BADPAT,	"BADPAT",	"invalid regular expression",
	REG_ECOLLATE,	"ECOLLATE",	"invalid collating element",
	REG_ECTYPE,	"ECTYPE",	"invalid character class",
	REG_EESCAPE,	"EESCAPE",	"\\ applied to unescapable character",
	REG_ESUBREG,	"ESUBREG",	"invalid backreference number",
	REG_EBRACK,	"EBRACK",	"brackets ([ ]) not balanced",
	REG_EPAREN,	"EPAREN",	"parentheses not balanced",
	REG_EBRACE,	"EBRACE",	"braces not balanced",
	REG_BADBR,	"BADBR",	"invalid repetition count(s)",
	REG_ERANGE,	"ERANGE",	"invalid character range",
	REG_ESPACE,	"ESPACE",	"out of memory",
	REG_BADRPT,	"BADRPT",	"repetition-operator operand invalid",
	REG_EMPTY,	"EMPTY",	"empty (sub)expression",
	REG_ASSERT,	"ASSERT",	"\"can't happen\" -- you found a bug",
	0,		"",		"*** unknown regexp error code ***",
};

/*
 - regerror - the interface to error numbers
 */
/* ARGSUSED */
size_t
regerror(errcode, preg, errbuf, errbuf_size)
int errcode;
const regex_t *preg;
char *errbuf;
size_t errbuf_size;
{
	register struct rerr *r;
	register size_t len;

	for (r = rerrs; r->code != 0; r++)
		if (r->code == errcode)
			break;

	len = strlen(r->explain) + 1;
	if (errbuf_size > 0) {
		if (errbuf_size > len)
			(void) strcpy(errbuf, r->explain);
		else {
			(void) strncpy(errbuf, r->explain, errbuf_size-1);
			errbuf[errbuf_size-1] = '\0';
		}
	}

	return(len);
}

#ifndef NDEBUG
/*
 - eprint - express an error number as a string
 */
char *
eprint(eno)
int eno;
{
	register struct rerr *r;
	static char eval[10];

	for (r = rerrs; r->code != 0; r++)
		if (r->code == eno)
			return(r->name);
	sprintf(eval, "#%d", r->code);
	return(eval);
}

/*
 - efind - find an error name
 */
int
efind(ename)
char *ename;
{
	register struct rerr *r;

	for (r = rerrs; r->code != 0; r++)
		if (strcmp(r->name, ename) == 0)
			return(r->code);
	return(0);		/* it'll do */
}
#endif
