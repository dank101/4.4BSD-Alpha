/*-
 * Copyright (c) 1980 The Regents of the University of California.
 * All rights reserved.
 *
 * This module is believed to contain source code proprietary to AT&T.
 * Use and redistribution is subject to the Berkeley Software License
 * Agreement and your Software Agreement with AT&T (Western Electric).
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)tell.c	5.3 (Berkeley) 4/19/91";
#endif /* LIBC_SCCS and not lint */

/*
 * return offset in file.
 */

long	lseek();

long tell(f)
{
	return(lseek(f, 0L, 1));
}
