#ifndef _REGEX_H
/* types */
typedef off_t regoff_t;
typedef struct {
	int re_magic;
	size_t re_nsub;		/* number of parenthesized subexpressions */
	struct re_guts *re_g;	/* none of your business :-) */
} regex_t;
typedef struct {
	regoff_t rm_so;		/* start of match */
	regoff_t rm_eo;		/* end of match */
} regmatch_t;

/* routines */
extern int regcomp(/*regex_t *preg, const char *pattern, int cflags*/);
extern int regexec(/*const regex_t *preg, const char *string, size_t nmatch,
					regmatch_t pmatch[], int eflags*/);
extern size_t regerror(/*int errcode, const regex_t *preg, char *errbuf,
							size_t errbuf_size*/);
extern void regfree(/*regex_t *preg*/);

/* regcomp() flags */
#define	REG_EXTENDED	01
#define	REG_ICASE	02
#define	REG_NOSUB	04
#define	REG_NEWLINE	010

/* regexec() flags */
#define	REG_NOTBOL	01
#define	REG_NOTEOL	02
#define	REG_STARTEND	04
#define	REG_TRACE	0400	/* debugging tracing */
#define	REG_LARGE	01000	/* force large state model for debug */

/* errors */
#define	REG_NOMATCH	(1)
#define	REG_BADPAT	(2)
#define	REG_ECOLLATE	(3)
#define	REG_ECTYPE	(4)
#define	REG_EESCAPE	(5)
#define	REG_ESUBREG	(6)
#define	REG_EBRACK	(7)
#define	REG_EPAREN	(8)
#define	REG_EBRACE	(9)
#define	REG_BADBR	(10)
#define	REG_ERANGE	(11)
#define	REG_ESPACE	(12)
#define	REG_BADRPT	(13)
#define	REG_EMPTY	(14)	/* empty component */
#define	REG_ASSERT	(15)	/* assertion failure */

#define	_REGEX_H	/* never again */
#endif
