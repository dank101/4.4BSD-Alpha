/* utility definitions */
#define	DUPMAX		_POSIX2_RE_DUP_MAX	/* xxx is this right? */
#define	INFINITY	(DUPMAX+1)
#define	NUC		(UCHAR_MAX+1)
typedef unsigned char uchar;

#ifndef STATIC
#define	STATIC	static		/* override if cc no like "static int f();" */
#endif
