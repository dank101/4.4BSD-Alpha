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

#include "gigi.h"

point(xi,yi)
int xi,yi;
{
	if(xsc(xi)!=currentx || ysc(yi)!=currenty)
		move(xi,yi);
	printf("V[]");
}
