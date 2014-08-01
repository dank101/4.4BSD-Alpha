/*-
 * Copyright (c) 1980, 1989, 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * This module is believed to contain source code proprietary to AT&T.
 * Use and redistribution is subject to the Berkeley Software License
 * Agreement and your Software Agreement with AT&T (Western Electric).
 *
 *	@(#)systat.h	5.8 (Berkeley) 7/24/92
 */

#include <curses.h>

struct  cmdtab {
        char    *c_name;		/* command name */
        void    (*c_refresh)();		/* display refresh */
        void    (*c_fetch)();		/* sets up data structures */
        void    (*c_label)();		/* label display */
	int	(*c_init)();		/* initialize namelist, etc. */
	WINDOW	*(*c_open)();		/* open display */
	void	(*c_close)();		/* close display */
	int	(*c_cmd)();		/* display command interpreter */
	char	c_flags;		/* see below */
};

#define	CF_INIT		0x1		/* been initialized */
#define	CF_LOADAV	0x2		/* display w/ load average */

#define	TCP	0x1
#define	UDP	0x2

#define KREAD(addr, buf, len)  kvm_ckread((addr), (buf), (len))
#define NVAL(indx)  nl[(indx)].n_value
#define NPTR(indx)  (void *)NVAL((indx))
#define NREAD(indx, buf, len) kvm_ckread(NPTR((indx)), (buf), (len))
#define LONG	(sizeof (long))
