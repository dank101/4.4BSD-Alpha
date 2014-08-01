/*-
 * Copyright (c) 1982, 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * This module is believed to contain source code proprietary to AT&T.
 * Use and redistribution is subject to the Berkeley Software License
 * Agreement and your Software Agreement with AT&T (Western Electric).
 *
 *	@(#)sys.c	7.17 (Berkeley) 5/27/92
 */

#include <sys/param.h>
#include <sys/reboot.h>
#include <ufs/ufs/dir.h>
#include "saio.h"

struct iob iob[SOPEN_MAX];
int errno;

#ifndef i386
exit()
{
	_stop("Exit called");
}
#endif

_stop(s)
	char *s;
{
	static int stopped;
	int i;

	if (!stopped) {
		stopped = 1;
		for (i = 0; i < SOPEN_MAX; i++)
			if (iob[i].i_flgs != 0)
				(void)close(i);
	}
	printf("%s\n", s);
	_rtt();
}
