/*-
 * Copyright (c) 1980, 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * This module is believed to contain source code proprietary to AT&T.
 * Use and redistribution is subject to the Berkeley Software License
 * Agreement and your Software Agreement with AT&T (Western Electric).
 */

#ifndef lint
static char sccsid[] = "@(#)cmds.c	5.10 (Berkeley) 7/23/92";
#endif /* not lint */

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include "systat.h"
#include "extern.h"

void
command(cmd)
        char *cmd;
{
        register struct cmdtab *p;
        register char *cp;
	int interval, omask;

	omask = sigblock(sigmask(SIGALRM));
        for (cp = cmd; *cp && !isspace(*cp); cp++)
                ;
        if (*cp)
                *cp++ = '\0';
	if (*cmd == '\0')
		return;
	for (; *cp && isspace(*cp); cp++)
		;
        if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "q") == 0)
                die(0);
	if (strcmp(cmd, "load") == 0) {
		load();
		goto done;
	}
        if (strcmp(cmd, "stop") == 0) {
                alarm(0);
                mvaddstr(CMDLINE, 0, "Refresh disabled.");
                clrtoeol();
		goto done;
        }
	if (strcmp(cmd, "help") == 0) {
		int col, len;

		move(CMDLINE, col = 0);
		for (p = cmdtab; p->c_name; p++) {
			len = strlen(p->c_name);
			if (col + len > COLS)
				break;
			addstr(p->c_name); col += len;
			if (col + 1 < COLS)
				addch(' ');
		}
		clrtoeol();
		goto done;
	}
	interval = atoi(cmd);
        if (interval <= 0 &&
	    (strcmp(cmd, "start") == 0 || strcmp(cmd, "interval") == 0)) {
		interval = *cp ? atoi(cp) : naptime;
                if (interval <= 0) {
			error("%d: bad interval.", interval);
			goto done;
                }
	}
	if (interval > 0) {
                alarm(0);
                naptime = interval;
                display(0);
                status();
		goto done;
        }
	p = lookup(cmd);
	if (p == (struct cmdtab *)-1) {
		error("%s: Ambiguous command.", cmd);
		goto done;
	}
        if (p) {
                if (curcmd == p)
			goto done;
                alarm(0);
		(*curcmd->c_close)(wnd);
		wnd = (*p->c_open)();
		if (wnd == 0) {
			error("Couldn't open new display");
			wnd = (*curcmd->c_open)();
			if (wnd == 0) {
				error("Couldn't change back to previous cmd");
				exit(1);
			}
			p = curcmd;
		}
		if ((p->c_flags & CF_INIT) == 0) {
			if ((*p->c_init)())
				p->c_flags |= CF_INIT;
			else
				goto done;
		}
                curcmd = p;
		labels();
                display(0);
                status();
		goto done;
        }
	if (curcmd->c_cmd == 0 || !(*curcmd->c_cmd)(cmd, cp))
		error("%s: Unknown command.", cmd);
done:
	sigsetmask(omask);
}

struct cmdtab *
lookup(name)
	register char *name;
{
	register char *p, *q;
	register struct cmdtab *c, *found;
	register int nmatches, longest;

	longest = 0;
	nmatches = 0;
	found = (struct cmdtab *) 0;
	for (c = cmdtab; p = c->c_name; c++) {
		for (q = name; *q == *p++; q++)
			if (*q == 0)		/* exact match? */
				return (c);
		if (!*q) {			/* the name was a prefix */
			if (q - name > longest) {
				longest = q - name;
				nmatches = 1;
				found = c;
			} else if (q - name == longest)
				nmatches++;
		}
	}
	if (nmatches > 1)
		return ((struct cmdtab *)-1);
	return (found);
}

void
status()
{

        error("Showing %s, refresh every %d seconds.",
          curcmd->c_name, naptime);
}

void
suspend(signo)
	int signo;
{
	extern sig_t sigtstpdfl;
        int oldmask;

	alarm(0);
        move(CMDLINE, 0);
        refresh();
        echo();
        nocrmode();
        (void)signal(SIGTSTP, sigtstpdfl);
        oldmask = sigsetmask(0);
        kill(getpid(), SIGTSTP);
        sigsetmask(oldmask);
        (void)signal(SIGTSTP, suspend);
        crmode();
        noecho();
        move(CMDLINE, col);
	alarm(naptime);
}

int
prefix(s1, s2)
        register char *s1, *s2;
{

        while (*s1 == *s2) {
                if (*s1 == '\0')
                        return (1);
                s1++, s2++;
        }
        return (*s1 == '\0');
}
