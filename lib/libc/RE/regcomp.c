#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <regex.h>

#include "utils.h"
#include "regex2.h"

#include "cclass.h"
#include "cname.h"

static uchar nuls[10];		/* place to point scanner in event of error */

/*
 * parse structure, passed up and down to avoid global variables and
 * other clumsinesses
 */
struct parse {
	uchar *next;		/* next character in RE */
	int error;		/* has an error been seen? */
	sop *strip;		/* malloced strip */
	sopno ssize;		/* malloced strip size (allocated) */
	sopno slen;		/* malloced strip length (used) */
	int ncsalloc;		/* number of csets allocated */
	struct re_guts *g;
#	define	NPAREN	10	/* we need to remember () 1-9 for back refs */
	sopno pbegin[NPAREN];	/* -> ( ([0] unused) */
	sopno pend[NPAREN];	/* -> ) ([0] unused) */
};

/*
 * macros for use with parse structure
 * BEWARE:  these know that the parse structure is named `p' !!!
 */
STATIC void doemit();
STATIC void doinsert();
STATIC void dofwd();
STATIC int seterr();
#define	PEEK()	((uchar)*p->next)
#define	PEEK2()	((uchar)*(p->next+1))
#define	SEE(c)	(PEEK() == (c))
#define	SEETWO(a, b)	(PEEK() == (a) && PEEK2() == (b))
#define	EAT(c)	((SEE(c)) ? (NEXT(), 1) : 0)
#define	EATTWO(a, b)	((SEETWO(a, b)) ? (NEXT2(), 1) : 0)
#define	NEXT()	(p->next++)
#define	NEXT2()	(p->next += 2)
#define	NEXTn(n)	(p->next += (n))
#define	GETNEXT()	((uchar)*p->next++)
#define	SETERROR(e)	seterr(p, (e))
#define	REQUIRE(co, e)	((co) || SETERROR(e))
#define	MUSTSEE(c, e)	(REQUIRE(PEEK() == (c), e))
#define	MUSTEAT(c, e)	(REQUIRE(GETNEXT() == (c), e))
#define	MUSTNOTSEE(c, e)	(REQUIRE(PEEK() != (c), e))
#define	EMIT(sop, sopnd)	doemit(p, sop, (size_t)(sopnd))
#define	INSERT(sop, pos)	doinsert(p, sop, HERE()-(pos)+1, pos)
#define	FWD(pos)		dofwd(p, pos, HERE()-(pos))
#define	BACK(sop, pos)		EMIT(sop, HERE()-pos)
#define	HERE()		(p->slen)
#define	THERE()		(p->slen - 1)
#define	DROP(n)	(p->slen -= (n))

STATIC void enlarge();
STATIC void mcadd();
STATIC void mcsub();
STATIC int mcin();
STATIC void mcinvert();
STATIC uchar *mcfind();

/*
 - regcomp - interface for parser and compilation
 */
int				/* 0 success, otherwise REG_something */
regcomp(preg, pattern, cflags)
regex_t *preg;
const char *pattern;
int cflags;
{
	struct parse pa;
	register struct re_guts *g;
	register struct parse *p = &pa;
	sopno nstates;
	register int i;
	STATIC void p_ere();
	STATIC void p_bre();
	STATIC void stripsnug();
	STATIC void categorize();
	STATIC void findmust();
	STATIC sopno pluscount();

	/* do the mallocs early so failure handling is easy */
	/* the +NUC here is for the category table */
	g = (struct re_guts *)malloc(sizeof(struct re_guts) + NUC);
	if (g == NULL)
		return(REG_ESPACE);
	p->ssize = strlen(pattern)/2*3 + 1;
	p->strip = (sop *)malloc(p->ssize * sizeof(sop));
	p->slen = 0;
	if (p->strip == NULL) {
		free((char *)g);
		return(REG_ESPACE);
	}

	/* set things up */
	p->g = g;
	p->next = (uchar *)pattern;
	p->error = 0;
	p->ncsalloc = 0;
	for (i = 0; i < NPAREN; i++) {
		p->pbegin[i] = 0;
		p->pend[i] = 0;
	}
	g->csetsize = NUC;
	g->sets = NULL;
	g->setbits = NULL;
	g->ncsets = 0;
	g->cflags = cflags;
	g->iflags = 0;
	g->must = NULL;
	g->mlen = 0;
	g->nsub = 0;
	g->ncategories = 1;	/* category 0 is "everything else" */
	g->categories = (uchar *)g + sizeof(struct re_guts);
	(void) memset((char *)g->categories, 0, NUC);
	g->backrefs = 0;
	g->nplus = 0;

	/* do it */
	EMIT(OEND, 0);
	g->firststate = THERE();
	if (cflags&REG_EXTENDED)
		p_ere(p, '\0');
	else
		p_bre(p, '\0', '\0');
	EMIT(OEND, 0);
	g->laststate = THERE();

	/* tidy up loose ends and fill things in */
	categorize(p, g);
	stripsnug(p, g);
	findmust(p, g);
	g->nplus = pluscount(p, g);
	g->magic = MAGIC2;
	preg->re_nsub = g->nsub;
	preg->re_g = g;
	preg->re_magic = MAGIC1;
#ifdef NDEBUG
	/* not debugging, so can't rely on the assert() in regexec() */
	if (g->iflags&BAD)
		SETERROR(REG_ASSERT);
#endif

	/* win or lose, we're done */
	if (p->error != 0)	/* lose */
		regfree(preg);
	return(p->error);
}

/*
 - p_ere - ERE parser top level, concatenation and alternation
 */
static void
p_ere(p, stop)
register struct parse *p;
uchar stop;			/* character this ERE should end at */
{
	STATIC void p_ere_exp();
	register uchar c;
	register sopno prevback;
	register sopno prevfwd;
	register sopno conc;
	register int first = 1;		/* is this the first alternative? */

	for (;;) {
		/* do a bunch of concatenated expressions */
		conc = HERE();
		while ((c = PEEK()) != '|' && c != stop && c != '\0')
			p_ere_exp(p);
		REQUIRE(HERE() != conc, REG_EMPTY);	/* require nonempty */

		if (!EAT('|'))
			break;		/* NOTE BREAK OUT */

		if (first) {
			INSERT(OCH_, conc);	/* offset is wrong */
			prevfwd = conc;
			prevback = conc;
			first = 0;
		}
		BACK(OOR1, prevback);
		prevback = THERE();
		FWD(prevfwd);			/* fix previous offset */
		prevfwd = HERE();
		EMIT(OOR2, 0);			/* offset is very wrong */
	}

	if (!first) {		/* tail-end fixups */
		FWD(prevfwd);
		BACK(O_CH, prevback);
	}

	assert(SEE(stop) || SEE('\0'));
}

/*
 - p_ere_exp - parse one subERE, an atom possibly followed by a repetition op
 */
static void
p_ere_exp(p)
register struct parse *p;
{
	STATIC int p_count();
	STATIC void p_bracket();
	STATIC void ordinary();
	STATIC void nonnewline();
	STATIC void repeat();
	register uchar c;
	register sopno pos;
	register sopno sub;
	register int count;
	register int count2;
	register sopno subno;
	int wascaret = 0;
	/* we call { a repetition if followed by a digit */
#	define	ISRPT(c1, c2)	(c1 == '*' || c1 == '+' || c1 == '?' || \
						(c1 == '{' && isdigit(c2)))

	c = GETNEXT();
	assert(c != '\0');	/* caller should have ensured this */

	pos = HERE();
	switch (c) {
	case '(':
		MUSTNOTSEE('\0', REG_EPAREN);
		p->g->nsub++;
		subno = p->g->nsub;
		if (subno < NPAREN)
			p->pbegin[subno] = HERE();
		EMIT(OLPAREN, subno);
		if (!SEE(')'))
			p_ere(p, ')');
		if (subno < NPAREN) {
			p->pend[subno] = HERE();
			assert(p->pend[subno] != 0);
		}
		EMIT(ORPAREN, subno);
		MUSTEAT(')', REG_EPAREN);
		break;
#ifndef POSIX_MISTAKE
	case ')':		/* happens only if no current unmatched ( */
		/*
		 * You may ask, why the ifndef?  Because I didn't notice
		 * this until slightly too late for 1003.2, and none of the
		 * other 1003.2 regular-expression reviewers noticed it at
		 * all.  So an unmatched ) is legal POSIX, at least until
		 * we can get it fixed.
		 */
		SETERROR(REG_EPAREN);
		break;
#endif
	case '^':
		EMIT(OBOL, 0);
		p->g->iflags |= USEBOL;
		wascaret = 1;
		break;
	case '$':
		EMIT(OEOL, 0);
		p->g->iflags |= USEEOL;
		break;
	case '|':
		SETERROR(REG_EMPTY);
		break;
	case '*':
	case '+':
	case '?':
		SETERROR(REG_BADRPT);
		break;
	case '.':
		if (p->g->cflags&REG_NEWLINE)
			nonnewline(p);
		else
			EMIT(OANY, 0);
		break;
	case '[':
		p_bracket(p);
		break;
	case '\\':
		c = GETNEXT();
		if (c == '^' || c == '.' || c == '[' || c == '$' ||
				c == '(' || c == ')' || c == '|' ||
				c == '*' || c == '+' || c == '?' ||
				c == '{' || c == '\\')
			ordinary(p, c);
		else
			SETERROR(REG_EESCAPE);
		break;
	case '{':		/* okay as ordinary except if digit follows */
		REQUIRE(!isdigit(PEEK()), REG_BADRPT);
		/* FALLTHROUGH */
	default:
		ordinary(p, c);
		break;
	}

	c = PEEK();
	if (!ISRPT(c, PEEK2()))
		return;		/* no repetition, we're done */
	NEXT();

	REQUIRE(!wascaret, REG_BADRPT);
	switch (c) {
	case '*':	/* implemented as +? */
		INSERT(OPLUS_, pos);
		BACK(O_PLUS, pos);
		INSERT(OQUEST_, pos);
		BACK(O_QUEST, pos);
		break;
	case '+':
		INSERT(OPLUS_, pos);
		BACK(O_PLUS, pos);
		break;
	case '?':
		INSERT(OQUEST_, pos);
		BACK(O_QUEST, pos);
		break;
	case '{':
		count = p_count(p);
		if (EAT(',')) {
			if (isdigit(PEEK())) {
				count2 = p_count(p);
				REQUIRE(count <= count2, REG_BADBR);
			} else		/* single number with comma */
				count2 = INFINITY;
		} else		/* just a single number */
			count2 = count;
		repeat(p, pos, count, count2);
		if (!EAT('}')) {	/* error heuristics */
			while ((c = PEEK()) != '\0' && c != '}')
				NEXT();
			if (c == '\0')
				SETERROR(REG_EBRACE);
			else
				SETERROR(REG_BADBR);
		}
		break;
	}

	c = PEEK();
	REQUIRE(!ISRPT(c, PEEK2()), REG_BADRPT);
}

/*
 - p_bre - BRE parser top level, anchoring and concatenation
 *
 * Giving end1 as '\0' essentially eliminates the end1/end2 check.
 */
static void
p_bre(p, end1, end2)
register struct parse *p;
register uchar end1;		/* first terminating character */
register uchar end2;		/* second terminating character */
{
	STATIC int p_simp_re();
	register sopno start = HERE();
	register int first = 1;			/* first subexpression? */
	register int wasdollar;

	if (EAT('^')) {
		EMIT(OBOL, 0);
		p->g->iflags |= USEBOL;
	}
	while (!SEE('\0') && !SEETWO(end1, end2)) {
		wasdollar = p_simp_re(p, first);
		first = 0;
	}
	if (wasdollar) {	/* oops, that was a trailing anchor */
		DROP(1);
		EMIT(OEOL, 0);
		p->g->iflags |= USEEOL;
	}

	REQUIRE(HERE() != start, REG_EMPTY);	/* require nonempty */
}

/*
 - p_simp_re - parse a simple RE, an atom possibly followed by a repetition
 */
static int			/* was the simple RE an unbackslashed $? */
p_simp_re(p, starordinary)
register struct parse *p;
int starordinary;		/* is a leading * an ordinary character? */
{
	STATIC int p_count();
	STATIC void p_bracket();
	STATIC void ordinary();
	STATIC void nonnewline();
	STATIC void repeat();
	register int c;
	register int count;
	register int count2;
	register sopno pos;
	register sopno sub;
	register int i;
	register sopno subno;
#	define	BACKSL	(1<<CHAR_BITS)

	pos = HERE();		/* repetion op, if any, covers from here */

	c = GETNEXT();
	assert(c != '\0');	/* caller should have ensured this */
	if (c == '\\')
		c = BACKSL | PEEK();
	switch (c) {
	case '.':
		if (p->g->cflags&REG_NEWLINE)
			nonnewline(p);
		else
			EMIT(OANY, 0);
		break;
	case '[':
		p_bracket(p);
		break;
	case BACKSL|'^':
	case BACKSL|'.':
	case BACKSL|'*':
	case BACKSL|'[':
	case BACKSL|'$':
	case BACKSL|'\\':
		ordinary(p, c&~BACKSL);
		NEXT();
		break;
	case BACKSL|'{':
		SETERROR(REG_BADRPT);
		break;
	case BACKSL|'(':
		NEXT();
		p->g->nsub++;
		subno = p->g->nsub;
		if (subno < NPAREN)
			p->pbegin[subno] = HERE();
		EMIT(OLPAREN, subno);
		/* the SEE here is an error heuristic */
		if (!SEE('\0') && !SEETWO('\\', ')'))
			p_bre(p, '\\', ')');
		if (subno < NPAREN) {
			p->pend[subno] = HERE();
			assert(p->pend[subno] != 0);
		}
		EMIT(ORPAREN, subno);
		REQUIRE(EATTWO('\\', ')'), REG_EPAREN);
		break;
	case BACKSL|')':	/* should not get here -- must be user */
	case BACKSL|'}':
		SETERROR(REG_EPAREN);
		break;
	case BACKSL|'1':
	case BACKSL|'2':
	case BACKSL|'3':
	case BACKSL|'4':
	case BACKSL|'5':
	case BACKSL|'6':
	case BACKSL|'7':
	case BACKSL|'8':
	case BACKSL|'9':
		i = (c&~BACKSL) - '0';
		assert(i < NPAREN);
		if (p->pend[i] != 0) {
			assert(i <= p->g->nsub);
			EMIT(OBACK_, i);
			assert(p->pbegin[i] != 0);
			assert(OP(p->strip[p->pbegin[i]]) == OLPAREN);
			assert(OP(p->strip[p->pend[i]]) == ORPAREN);
			(void) dupl(p, p->pbegin[i]+1, p->pend[i]);
			EMIT(O_BACK, i);
		} else
			SETERROR(REG_ESUBREG);
		p->g->backrefs = 1;
		NEXT();
		break;
	case '*':
		REQUIRE(starordinary, REG_BADRPT);
		/* FALLTHROUGH */
	default:
		if (c & BACKSL) {
			SETERROR(REG_EESCAPE);
			c &= ~BACKSL;
		}
		ordinary(p, (uchar)c);
		break;
	}

	if (EAT('*')) {		/* implemented as +? */
		INSERT(OPLUS_, pos);
		BACK(O_PLUS, pos);
		INSERT(OQUEST_, pos);
		BACK(O_QUEST, pos);
	} else if (EATTWO('\\', '{')) {
		count = p_count(p);
		if (EAT(',')) {
			if (isdigit(PEEK())) {
				count2 = p_count(p);
				REQUIRE(count <= count2, REG_BADBR);
			} else		/* single number with comma */
				count2 = INFINITY;
		} else		/* just a single number */
			count2 = count;
		repeat(p, pos, count, count2);
		if (!EATTWO('\\', '}')) {	/* error heuristics */
			while (!SEE('\0') && !SEETWO('\\', '}'))
				NEXT();
			if (SEE('\0'))
				SETERROR(REG_EBRACE);
			else
				SETERROR(REG_BADBR);
		}
	} else if (c == '$')	/* unbackslashed $ not followed by reptn */
		return(1);

	return(0);
}

/*
 - p_count - parse a repetition count
 */
static int			/* the value */
p_count(p)
register struct parse *p;
{
	register int count = 0;
	register int ndigits = 0;

	while (isdigit(PEEK()) && count <= DUPMAX) {
		count = count*10 + (GETNEXT() - '0');
		ndigits++;
	}

	REQUIRE(ndigits > 0 && count <= DUPMAX, REG_BADBR);
	return(count);
}

/*
 - p_bracket - parse a bracketed character list
 *
 * Note a significant property of this code:  if the allocset() did SETERROR,
 * no set operations are done.
 */
static void
p_bracket(p)
register struct parse *p;
{
	STATIC void p_b_term();
	STATIC cset *allocset();
	STATIC int freezeset();
	register uchar c;
	register cset *cs = allocset(p);
	register int invert = 0;

	if (EAT('^'))
		invert++;	/* make note to invert set at end */
	if (EAT(']'))
		CHadd(cs, ']');
	while ((c = PEEK()) != '\0' && c != ']' && !SEETWO('-', ']'))
		p_b_term(p, cs);
	if (EAT('-'))
		CHadd(cs, '-');
	MUSTEAT(']', REG_EBRACK);

	if (invert && p->error == 0) {
		register int i;

		for (i = p->g->csetsize - 1; i >= 0; i--)
			if (CHIN(cs, i))
				CHsub(cs, i);
			else
				CHadd(cs, i);
		if (p->g->cflags&REG_NEWLINE)
			CHsub(cs, '\n');
		if (cs->multis != NULL)
			mcinvert(p, cs);
	}
	assert(cs->multis == NULL);		/* xxx */
	EMIT(OANYOF, freezeset(p, cs));
}

/*
 - p_b_term - parse one term of a bracketed character list
 */
static void
p_b_term(p, cs)
register struct parse *p;
register cset *cs;
{
	STATIC uchar p_b_symbol();
	STATIC void p_b_cclass();
	STATIC void p_b_eclass();
	STATIC uchar othercase();
	register uchar c;
	register uchar start, finish;
	register int i;

	/* classify what we've got */
	switch (PEEK()) {
	case '[':
		c = PEEK2();
		break;
	case '-':
		SETERROR(REG_ERANGE);
		return;			/* NOTE RETURN */
		break;
	default:
		c = '\0';
		break;
	}

	switch (c) {
	case ':':		/* character class */
		NEXT2();
		c = PEEK();
		REQUIRE(c != '\0', REG_EBRACK);
		REQUIRE(c != '-' && c != ']', REG_ECTYPE);
		p_b_cclass(p, cs);
		MUSTNOTSEE('\0', REG_EBRACK);
		REQUIRE(EATTWO(':', ']'), REG_ECTYPE);
		break;
	case '=':		/* equivalence class */
		NEXT2();
		c = PEEK();
		REQUIRE(c != '\0', REG_EBRACK);
		REQUIRE(c != '-' && c != ']', REG_ECOLLATE);
		p_b_eclass(p, cs);
		MUSTNOTSEE('\0', REG_EBRACK);
		REQUIRE(EATTWO('=', ']'), REG_ECOLLATE);
		break;
	default:		/* symbol, ordinary character, or range */
/* xxx revision needed for multichar stuff */
		start = p_b_symbol(p);
		if (PEEK() == '-' && (c = PEEK2()) != ']' && c != '\0') {
			/* range */
			NEXT();
			if (EAT('-'))
				finish = '-';
			else
				finish = p_b_symbol(p);
		} else
			finish = start;
		REQUIRE(start <= finish, REG_ERANGE);
		for (i = start; i <= finish; i++) {
			CHadd(cs, i);
			if ((p->g->cflags&REG_ICASE) && isalpha(i)) {
				c = othercase((uchar)i);
				CHadd(cs, c);
			}
		}
		break;
	}
}

/*
 - p_b_cclass - parse a character-class name and deal with it
 */
static void
p_b_cclass(p, cs)
register struct parse *p;
register cset *cs;
{
	register uchar *sb = p->next;
	register uchar *se = sb;
	register struct cclass *cp;
	register int len;
	register uchar *u;
	register uchar c;

	while (isalpha(*se))
		se++;
	len = se - sb;
	NEXTn(len);
	for (cp = cclasses; cp->name != NULL; cp++)
		if (strncmp(cp->name, sb, len) == 0 && cp->name[len] == '\0')
			break;
	if (cp->name == NULL) {
		/* oops, didn't find it */
		SETERROR(REG_ECTYPE);
		return;
	}

	u = (uchar *)cp->chars;
	while ((c = *u++) != '\0')
		CHadd(cs, c);
	for (u = (uchar *)cp->multis; *u != '\0'; u += strlen((char *)u) + 1)
		MCadd(cs, u);
}

/*
 - p_b_eclass - parse an equivalence-class name and deal with it
 *
 * This implementation is incomplete. xxx
 */
static void
p_b_eclass(p, cs)
register struct parse *p;
register cset *cs;
{
	register uchar c;
	STATIC uchar p_b_coll_elem();

	c = p_b_coll_elem(p, '=');
	CHadd(cs, c);
}

/*
 - p_b_symbol - parse a character or [..]ed multicharacter collating symbol
 */
static uchar			/* value of symbol */
p_b_symbol(p)
register struct parse *p;
{
	STATIC uchar p_b_coll_elem();
	register uchar value;

	if (!EATTWO('[', '.')) {
		MUSTNOTSEE('\0', REG_EBRACK);
		return(GETNEXT());
	}

	/* collating symbol */
	MUSTNOTSEE('\0', REG_EBRACK);
	value = p_b_coll_elem(p, '.');
	REQUIRE(EATTWO('.', ']'), REG_ECOLLATE);
	return(value);
}

/*
 - p_b_coll_elem - parse a collating-element name and look it up
 */
static uchar			/* value of collating element */
p_b_coll_elem(p, endc)
register struct parse *p;
uchar endc;			/* name ended by endc,']' */
{
	register uchar *sp = p->next;
	register struct cname *cp;
	register int len;
	register uchar c;

	while ((c = PEEK()) != '\0' && !SEETWO(endc, ']'))
		NEXT();
	if (c == '\0') {
		SETERROR(REG_EBRACK);
		return(0);
	}
	len = p->next - sp;
	for (cp = cnames; cp->name != NULL; cp++)
		if (strncmp(cp->name, sp, len) == 0 && cp->name[len] == '\0')
			return(cp->code);	/* known name */
	if (len == 1)
		return(*sp);	/* single character */
	SETERROR(REG_ECOLLATE);			/* neither */
	return(0);
}

/*
 - othercase - return the case counterpart of an alphabetic
 */
static uchar
othercase(ch)
uchar ch;
{
	assert(isalpha(ch));
	if (isupper(ch))
		return(tolower(ch));
	else if (islower(ch))
		return(toupper(ch));
	else			/* peculiar, but could happen */
		return(ch);
}

/*
 - bothcases - emit a dualcase version of a character
 * Boy, is this implementation ever a kludge...
 */
static void
bothcases(p, ch)
register struct parse *p;
uchar ch;
{
	register uchar *oldnext;
	uchar bracket[3];

	oldnext = p->next;
	p->next = bracket;
	bracket[0] = ch;
	bracket[1] = ']';
	bracket[2] = '\0';
	p_bracket(p);
	assert(p->next == bracket+2);
	p->next = oldnext;
}

/*
 - ordinary - emit an ordinary character
 */
static void
ordinary(p, ch)
register struct parse *p;
register uchar ch;
{
	register uchar *cap = p->g->categories;

	if ((p->g->cflags&REG_ICASE) && isalpha(ch)) {
		bothcases(p, ch);
		return;
	}

	EMIT(OCHAR, ch);
	if (cap[ch] == 0)
		cap[ch] = p->g->ncategories++;
}

/*
 - nonnewline - emit REG_NEWLINE version of OANY
 * Boy, is this implementation ever a kludge...
 */
static void
nonnewline(p)
register struct parse *p;
{
	register uchar *oldnext;
	uchar bracket[4];

	oldnext = p->next;
	p->next = bracket;
	bracket[0] = '^';
	bracket[1] = '\n';
	bracket[2] = ']';
	bracket[3] = '\0';
	p_bracket(p);
	assert(p->next == bracket+3);
	p->next = oldnext;
}

/*
 - repeat - generate code for a bounded repetition, recursively if needed
 */
static void
repeat(p, start, from, to)
register struct parse *p;
sopno start;			/* operand from here to end of strip */
int from;			/* repeated from this number */
int to;				/* to this number of times (maybe INFINITY) */
{
	register sopno finish = HERE();
#	define	N	2
#	define	INF	3
#	define	REP(f, t)	((f)*8 + (t))
#	define	MAP(n)	(((n) <= 1) ? (n) : ((n) == INFINITY) ? INF : N)
	STATIC sopno dupl();
	register sopno copy;

	if (p->error != 0)	/* head off possible runaway recursion */
		return;

	assert(from <= to);

	switch (REP(MAP(from), MAP(to))) {
	case REP(0, 0):			/* must be user doing this */
		DROP(finish-start);	/* drop the operand */
		break;
	case REP(0, 1):			/* as x{1,1}? */
	case REP(0, N):			/* as x{1,n}? */
	case REP(0, INF):		/* as x{1,}? */
		INSERT(OQUEST_, start);		/* offset is wrong... */
		repeat(p, start+1, 1, to);
		FWD(start);			/* ... fix it */
		BACK(O_QUEST, start);
		break;
	case REP(1, 1):			/* trivial case */
		/* done */
		break;
	case REP(1, N):			/* as x?x{1,n-1} */
		INSERT(OQUEST_, start);
		BACK(O_QUEST, start);
		copy = dupl(p, start+1, finish+1);
		assert(copy == finish+2);
		repeat(p, copy, 1, to-1);
		break;
	case REP(1, INF):		/* as x+ */
		INSERT(OPLUS_, start);
		BACK(O_PLUS, start);
		break;
	case REP(N, N):			/* as xx{m-1,n-1} */
		copy = dupl(p, start, finish);
		repeat(p, copy, from-1, to-1);
		break;
	case REP(N, INF):		/* as xx{n-1,INF} */
		copy = dupl(p, start, finish);
		repeat(p, copy, from-1, to);
		break;
	default:			/* "can't happen" */
		SETERROR(REG_ASSERT);	/* just in case */
		break;
	}
}

/*
 - seterr - set an error condition
 */
static int			/* useless but makes type checking happy */
seterr(p, e)
register struct parse *p;
int e;
{
	if (p->error == 0)	/* keep earliest error condition */
		p->error = e;
	p->next = nuls;		/* try to bring things to a halt */
	return(0);		/* make the return value well-defined */
}

/*
 - allocset - allocate a set of characters for []
 */
static cset *
allocset(p)
register struct parse *p;
{
	register int no = p->g->ncsets++;
	register size_t nc;
	register size_t nbytes;
	register cset *cs;
	register int i;
	register size_t css = (size_t)p->g->csetsize;

	if (no >= p->ncsalloc) {	/* need another column of space */
		p->ncsalloc += CHAR_BITS;
		nc = p->ncsalloc;
		assert(nc % CHAR_BITS == 0);
		nbytes = nc / CHAR_BITS * css;
		if (p->g->sets == NULL)
			p->g->sets = (cset *)malloc(nc * sizeof(cset));
		else
			p->g->sets = (cset *)realloc((char *)p->g->sets,
							nc * sizeof(cset));
		if (p->g->setbits == NULL)
			p->g->setbits = (uchar *)malloc(nbytes);
		else
			p->g->setbits = (uchar *)realloc((char *)p->g->setbits,
								nbytes);
		if (p->g->sets != NULL && p->g->setbits != NULL)
			(void) memset((char *)p->g->setbits + (nbytes - css),
								0, css);
		else {
			no = 0;
			SETERROR(REG_ESPACE);
			/* caller's responsibility not to do set ops */
		}
	}

	assert(p->g->sets != NULL);	/* xxx */
	cs = &p->g->sets[no];
	cs->ptr = p->g->setbits + css*((no)/CHAR_BITS);
	cs->mask = 1 << ((no) % CHAR_BITS);
	cs->hash = 0;
	cs->smultis = 0;
	cs->multis = NULL;

	return(cs);
}

/*
 - freezeset - final processing on a set of characters
 *
 * The main task here is merging identical sets.  This is usually a waste
 * of time (although the hash code minimizes the overhead), but can win
 * big if REG_ICASE is being used.  REG_ICASE, by the way, is why the hash
 * is done using addition rather than xor -- all ASCII [aA] sets xor to
 * the same value!
 */
static int			/* set number */
freezeset(p, cs)
register struct parse *p;
register cset *cs;
{
	register uchar h = cs->hash;
	register int i;
	register cset *top = &p->g->sets[p->g->ncsets];
	register uchar c;
	register cset *cs2;
	register size_t css = (size_t)p->g->csetsize;

	/* look for an earlier one which is the same */
	for (cs2 = &p->g->sets[0]; cs2 < top; cs2++)
		if (cs2->hash == h && cs2 != cs) {
			/* maybe */
			for (i = 0; i < css; i++)
				if (!!CHIN(cs2, i) != !!CHIN(cs, i))
					break;		/* no */
			if (i == css)
				break;			/* yes */
		}

	if (cs2 < top) {	/* found one */
		assert(cs == top-1);
		p->g->ncsets--;
		for (i = 0; i < css; i++)
			CHsub(cs, i);
		cs = cs2;
	}

	return((int)(cs - p->g->sets));
}

/*
 - mcadd - add a collating element to a cset
 */
static void
mcadd(p, cs, cp)
register struct parse *p;
register cset *cs;
register uchar *cp;
{
	register size_t oldend = cs->smultis;

	cs->smultis += strlen(cp) + 1;
	if (cs->multis == NULL)
		cs->multis = (uchar *)malloc(cs->smultis);
	else
		cs->multis = (uchar *)realloc(cs->multis, cs->smultis);
	if (cs->multis == NULL) {
		SETERROR(REG_ESPACE);
		return;
	}

	(void) strcpy((char *)(cs->multis + oldend - 1), (char *)cp);
	cs->multis[cs->smultis - 1] = '\0';
}

/*
 - mcsub - subtract a collating element from a cset
 */
static void
mcsub(p, cs, cp)
register struct parse *p;
register cset *cs;
register uchar *cp;
{
	register uchar *fp = mcfind(cs, cp);
	register size_t len = strlen((char *)fp);

	assert(p != NULL);
	(void) memmove((char *)fp, (char *)(fp + len + 1),
				cs->smultis - (fp + len + 1 - cs->multis));
	cs->smultis -= len;

	if (cs->smultis == 0) {
		free((char *)cs->multis);
		cs->multis = NULL;
		return;
	}

	cs->multis = (uchar *)realloc(cs->multis, cs->smultis);
	assert(cs->multis != NULL);
}

/*
 - mcin - is a collating element in a cset?
 */
static int
mcin(p, cs, cp)
register struct parse *p;
register cset *cs;
register uchar *cp;
{
	return(mcfind(cs, cp) != NULL);
}

/*
 - mcfind - find a collating element in a cset
 */
static uchar *
mcfind(cs, cp)
register cset *cs;
register uchar *cp;
{
	register uchar *p;

	if (cs->multis == NULL)
		return(NULL);
	for (p = cs->multis; *p != '\0'; p += strlen((char *)p) + 1)
		if (strcmp((char *)cp, (char *)p) == 0)
			return(p);
	return(NULL);
}

/*
 - mcinvert - invert the list of collating elements in a cset
 *
 * This would have to know the set of possibilities.  Implementation
 * is deferred.
 */
static void
mcinvert(p, cs)
register struct parse *p;
register cset *cs;
{
	assert(cs->multis == NULL);	/* xxx */
}

/*
 - isinsets - is this character in any sets?
 */
static int			/* predicate */
isinsets(g, c)
register struct re_guts *g;
uchar c;
{
	register uchar *col;
	register int i;
	register int ncols = (g->ncsets+(CHAR_BITS-1)) / CHAR_BITS;

	for (i = 0, col = g->setbits; i < ncols; i++, col += g->csetsize)
		if (col[c] != 0)
			return(1);
	return(0);
}

/*
 - samesets - are these two characters in exactly the same sets?
 */
static int			/* predicate */
samesets(g, c1, c2)
register struct re_guts *g;
register uchar c1;
register uchar c2;
{
	register uchar *col;
	register int i;
	register int ncols = (g->ncsets+(CHAR_BITS-1)) / CHAR_BITS;

	for (i = 0, col = g->setbits; i < ncols; i++, col += g->csetsize)
		if (col[c1] != col[c2])
			return(0);
	return(1);
}

/*
 - categorize - sort out character categories
 */
static void
categorize(p, g)
struct parse *p;
register struct re_guts *g;
{
	register uchar *cats = g->categories;
	register unsigned c;
	register unsigned c2;
	register uchar cat;

	/* avoid making error situations worse */
	if (p->error != 0)
		return;

	for (c = 0; c < g->csetsize; c++)
		if (cats[c] == 0 && isinsets(g, c)) {
			cat = g->ncategories++;
			cats[c] = cat;
			for (c2 = c+1; c2 < g->csetsize; c2++)
				if (cats[c2] == 0 && samesets(g, c, c2))
					cats[c2] = cat;
		}
}

/*
 - dupl - emit a duplicate of a bunch of sops
 */
static sopno			/* start of duplicate */
dupl(p, start, finish)
register struct parse *p;
sopno start;			/* from here */
sopno finish;			/* to this less one */
{
	register int i;
	register sopno ret = HERE();
	register sopno len = finish - start;

	assert(finish >= start);
	if (len == 0)
		return(ret);
	enlarge(p, p->ssize + len);	/* this many unexpected additions */
	assert(p->ssize >= p->slen + len);
	(void) memcpy((char *)(p->strip + p->slen),
		(char *)(p->strip + start), (size_t)len*sizeof(sop));
	p->slen += len;
	return(ret);
}

/*
 - doemit - emit a strip operator
 *
 * It might seem better to implement this as a macro with a function as
 * hard-case backup, but it's just too big and messy unless there are
 * some changes to the data structures.  Maybe later.
 */
static void
doemit(p, op, opnd)
register struct parse *p;
sop op;
size_t opnd;
{
	/* avoid making error situations worse */
	if (p->error != 0)
		return;

	/* deal with oversize operands ("can't happen", more or less) */
	assert(opnd < 1<<OPSHIFT);

	/* deal with undersized strip */
	if (p->slen >= p->ssize)
		enlarge(p, (p->ssize+1) / 2 * 3);	/* +50% */
	assert(p->slen < p->ssize);

	/* finally, it's all reduced to the easy case */
	p->strip[p->slen++] = SOP(op, opnd);
}

/*
 - doinsert - insert a sop into the strip
 */
static void
doinsert(p, op, opnd, pos)
register struct parse *p;
sop op;
size_t opnd;
sopno pos;
{
	register sopno sn;
	register sop s;
	register int i;

	/* avoid making error situations worse */
	if (p->error != 0)
		return;

	sn = HERE();
	EMIT(op, opnd);		/* do checks, ensure space */
	assert(HERE() == sn+1);
	s = p->strip[sn];

	/* adjust paren pointers */
	assert(pos > 0);
	for (i = 1; i < NPAREN; i++) {
		if (p->pbegin[i] >= pos) {
			p->pbegin[i]++;
		}
		if (p->pend[i] >= pos) {
			p->pend[i]++;
		}
	}

	memmove((char *)&p->strip[pos+1], (char *)&p->strip[pos],
						(HERE()-pos-1)*sizeof(sop));
	p->strip[pos] = s;
}

/*
 - dofwd - complete a forward reference
 */
static void
dofwd(p, pos, value)
register struct parse *p;
register sopno pos;
sop value;
{
	/* avoid making error situations worse */
	if (p->error != 0)
		return;

	assert(value < 1<<OPSHIFT);
	p->strip[pos] = OP(p->strip[pos]) | value;
}

/*
 - enlarge - enlarge the strip
 */
static void
enlarge(p, size)
register struct parse *p;
register sopno size;
{
	register sop *sp;

	if (p->ssize >= size)
		return;

	sp = (sop *)realloc(p->strip, size*sizeof(sop));
	if (sp == NULL) {
		SETERROR(REG_ESPACE);
		return;
	}
	p->strip = sp;
	p->ssize = size;
}

/*
 - stripsnug - compact the strip
 */
static void
stripsnug(p, g)
register struct parse *p;
register struct re_guts *g;
{
	g->nstates = p->slen;
	g->strip = (sop *)realloc((sop *)p->strip, p->slen * sizeof(sop));
	if (g->strip == NULL) {
		SETERROR(REG_ESPACE);
		g->strip = p->strip;
	}
}

/*
 - findmust - fill in must and mlen with longest mandatory literal string
 *
 * This algorithm could do fancy things like analyzing the operands of |
 * for common subsequences.  Someday.  This code is simple and finds most
 * of the interesting cases.
 *
 * Note that must and mlen got initialized during setup.
 */
STATIC void
findmust(p, g)
struct parse *p;
register struct re_guts *g;
{
	register sop *scan;
	sop *start;
	register sop *newstart;
	register sopno newlen;
	register sop s;
	register char *cp;
	register sopno i;

	/* avoid making error situations worse */
	if (p->error != 0)
		return;

	/* find the longest OCHAR sequence in strip */
	newlen = 0;
	scan = g->strip + 1;
	do {
		s = *scan++;
		switch (OP(s)) {
		case OCHAR:		/* sequence member */
			if (newlen == 0)		/* new sequence */
				newstart = scan - 1;
			newlen++;
			break;
		case OPLUS_:		/* things that don't break one */
		case OLPAREN:
		case ORPAREN:
			break;
		case OQUEST_:		/* things that must be skipped */
		case OCH_:
			scan--;
			do {
				scan += OPND(s);
				s = *scan;
				/* assert() interferes w debug printouts */
				if (OP(s) != O_QUEST && OP(s) != O_CH &&
							OP(s) != OOR2) {
					g->iflags |= BAD;
					return;
				}
			} while (OP(s) != O_QUEST && OP(s) != O_CH);
			/* fallthrough */
		default:		/* things that break a sequence */
			if (newlen > g->mlen) {		/* ends one */
				start = newstart;
				g->mlen = newlen;
			}
			newlen = 0;
			break;
		}
	} while (OP(s) != OEND);

	if (g->mlen == 0)		/* there isn't one */
		return;

	/* turn it into a character string */
	g->must = malloc((size_t)g->mlen + 1);
	if (g->must == NULL) {		/* argh; just forget it */
		g->mlen = 0;
		return;
	}
	cp = g->must;
	scan = start;
	for (i = g->mlen; i > 0; i--) {
		while (OP(s = *scan++) != OCHAR)
			continue;
		*cp++ = OPND(s);
	}
	*cp++ = '\0';		/* just on general principles */
}

/*
 - pluscount - count + nesting
 */
STATIC sopno			/* nesting depth */
pluscount(p, g)
struct parse *p;
register struct re_guts *g;
{
	register sop *scan;
	register sop s;
	register sopno plusnest = 0;
	register sopno maxnest = 0;

	if (p->error != 0)
		return(0);	/* there may not be an OEND */

	scan = g->strip + 1;
	do {
		s = *scan++;
		switch (OP(s)) {
		case OPLUS_:
			plusnest++;
			break;
		case O_PLUS:
			if (plusnest > maxnest)
				maxnest = plusnest;
			plusnest--;
			break;
		}
	} while (OP(s) != OEND);
	if (plusnest != 0)
		g->iflags |= BAD;
	return(maxnest);
}
