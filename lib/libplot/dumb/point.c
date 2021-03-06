/*-
 * Copyright (c) 1980 The Regents of the University of California.
 * All rights reserved.
 *
 * This module is believed to contain source code proprietary to AT&T.
 * Use and redistribution is subject to the Berkeley Software License
 * Agreement and your Software Agreement with AT&T (Western Electric).
 */

#ifndef lint
static char sccsid[] = "@(#)point.c	5.2 (Berkeley) 4/22/91";
#endif /* not lint */

#include "dumb.h"

point(x, y)
	int x,y;
{
	scale(x, y);
	currentx = x;
	currenty = y;
	screenmat[currentx][currenty] = '*';
}
