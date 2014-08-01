/*
 * Copyright (c) 1983 Eric P. Allman
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char sccsid[] = "@(#)mci.c	5.4 (Berkeley) 7/20/92";
#endif /* not lint */

#include "sendmail.h"

/*
**  Mail Connection Information (MCI) Caching Module.
**
**	There are actually two separate things cached.  The first is
**	the set of all open connections -- these are stored in a
**	(small) list.  The second is stored in the symbol table; it
**	has the overall status for all hosts, whether or not there
**	is a connection open currently.
**
**	There should never be too many connections open (since this
**	could flood the socket table), nor should a connection be
**	allowed to sit idly for too long.
**
**	MaxMciCache is the maximum number of open connections that
**	will be supported.
**
**	MciCacheTimeout is the time (in seconds) that a connection
**	is permitted to survive without activity.
**
**	We actually try any cached connections by sending a NOOP
**	before we use them; if the NOOP fails we close down the
**	connection and reopen it.  Note that this means that a
**	server SMTP that doesn't support NOOP will hose the
**	algorithm -- but that doesn't seem too likely.
*/

MCI	**MciCache;		/* the open connection cache */
/*
**  MCI_CACHE -- enter a connection structure into the open connection cache
**
**	This may cause something else to be flushed.
**
**	Parameters:
**		mci -- the connection to cache.
**
**	Returns:
**		none.
*/

mci_cache(mci)
	register MCI *mci;
{
	register MCI **mcislot;
	extern MCI **mci_scan();

	if (MaxMciCache <= 0)
	{
		/* we don't support caching */
		return;
	}

	/*
	**  Find the best slot.  This may cause expired connections
	**  to be closed.
	*/

	mcislot = mci_scan(mci);

	/* if this is already cached, we are done */
	if (bitset(MCIF_CACHED, mci->mci_flags))
		return;

	/* otherwise we may have to clear the slot */
	if (*mcislot != NULL)
		mci_uncache(mcislot);

	*mcislot = mci;
	mci->mci_flags |= MCIF_CACHED;
}
/*
**  MCI_SCAN -- scan the cache, flush junk, and return best slot
**
**	Parameters:
**		savemci -- never flush this one.  Can be null.
**
**	Returns:
**		The LRU (or empty) slot.
*/

MCI **
mci_scan(savemci)
	MCI *savemci;
{
	time_t now;
	register MCI **bestmci;
	register MCI *mci;
	register int i;

	if (MciCache == NULL)
	{
		/* first call */
		MciCache = (MCI **) xalloc(MaxMciCache * sizeof *MciCache);
		return (&MciCache[0]);
	}

	now = curtime();
	bestmci = &MciCache[0];
	for (i = 0; i < MaxMciCache; i++)
	{
		mci = MciCache[i];
		if (mci == NULL || mci->mci_state == MCIS_CLOSED)
		{
			bestmci = &MciCache[i];
			continue;
		}
		if (mci->mci_lastuse + MciCacheTimeout < now && mci != savemci)
		{
			/* connection idle too long -- close it */
			bestmci = &MciCache[i];
			mci_uncache(bestmci);
			continue;
		}
		if (*bestmci == NULL)
			continue;
		if (mci->mci_lastuse < (*bestmci)->mci_lastuse)
			bestmci = &MciCache[i];
	}
	return bestmci;
}
/*
**  MCI_UNCACHE -- remove a connection from a slot.
**
**	May close a connection.
**
**	Parameters:
**		mcislot -- the slot to empty.
**
**	Returns:
**		none.
*/

mci_uncache(mcislot)
	register MCI **mcislot;
{
	register MCI *mci;
	extern ENVELOPE BlankEnvelope;

	mci = *mcislot;
	if (mci == NULL)
		return;
	*mcislot = NULL;
	mci->mci_flags &= ~MCIF_CACHED;

	message(Arpa_Info, "Closing connection to %s", mci->mci_host);

	/* only uses the envelope to flush the transcript file */
	if (mci->mci_state != MCIS_CLOSED)
		smtpquit(mci->mci_mailer, mci, &BlankEnvelope);
}
/*
**  MCI_FLUSH -- flush the entire cache
*/

mci_flush()
{
	register int i;

	if (MciCache == NULL)
		return;

	for (i = 0; i < MaxMciCache; i++)
		mci_uncache(&MciCache[i]);
}
/*
**  MCI_GET -- get information about a particular host
*/

MCI *
mci_get(host, m)
	char *host;
	MAILER *m;
{
	register MCI *mci;
	register STAB *s;

	s = stab(host, ST_MCI + m->m_mno, ST_ENTER);
	mci = &s->s_mci;
	mci->mci_host = s->s_name;

	if (tTd(42, 2))
	{
		printf("mci_get(%s %s): mci_state=%d, _flags=%x, _exitstat=%d, _errno=%d\n",
			host, m->m_name, mci->mci_state, mci->mci_flags,
			mci->mci_exitstat, mci->mci_errno);
	}

	/* try poking this to see if it is still usable */
	switch (mci->mci_state)
	{
	  case MCIS_OPEN:
		smtpnoop(mci);
		break;
	}

	return mci;
}
