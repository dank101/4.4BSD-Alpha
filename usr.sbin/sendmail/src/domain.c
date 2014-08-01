/*
 * Copyright (c) 1986 Eric P. Allman
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

#include "sendmail.h"

#ifndef lint
#ifdef NAMED_BIND
static char sccsid[] = "@(#)domain.c	5.34 (Berkeley) 6/23/92 (with name server)";
#else
static char sccsid[] = "@(#)domain.c	5.34 (Berkeley) 6/23/92 (without name server)";
#endif
#endif /* not lint */

#ifdef NAMED_BIND

#include <sys/param.h>
#include <errno.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <netdb.h>

typedef union {
	HEADER qb1;
	char qb2[PACKETSZ];
} querybuf;

static char hostbuf[MAXMXHOSTS*PACKETSZ];

getmxrr(host, mxhosts, localhost, rcode)
	char *host, **mxhosts, *localhost;
	int *rcode;
{
	extern int h_errno;
	register u_char *eom, *cp;
	register int i, j, n, nmx;
	register char *bp;
	HEADER *hp;
	querybuf answer;
	int ancount, qdcount, buflen, seenlocal;
	u_short pref, localpref, type, prefer[MAXMXHOSTS];

	errno = 0;
	n = res_search(host, C_IN, T_MX, (char *)&answer, sizeof(answer));
	if (n < 0)
	{
		if (tTd(8, 1))
			printf("getmxrr: res_search(%s) failed (errno=%d, h_errno=%d)\n",
			    (host == NULL) ? "<NULL>" : host, errno, h_errno);
		switch (h_errno)
		{
		  case NO_DATA:
		  case NO_RECOVERY:
			/* no MX data on this host */
			goto punt;

		  case HOST_NOT_FOUND:
			/* the host just doesn't exist */
			*rcode = EX_NOHOST;
			break;

		  case TRY_AGAIN:
			/* couldn't connect to the name server */
			if (!UseNameServer && errno == ECONNREFUSED)
				goto punt;

			/* it might come up later; better queue it up */
			*rcode = EX_TEMPFAIL;
			break;
		}

		/* irreconcilable differences */
		return (-1);
	}

	/* find first satisfactory answer */
	hp = (HEADER *)&answer;
	cp = (u_char *)&answer + sizeof(HEADER);
	eom = (u_char *)&answer + n;
	for (qdcount = ntohs(hp->qdcount); qdcount--; cp += n + QFIXEDSZ)
		if ((n = dn_skipname(cp, eom)) < 0)
			goto punt;
	nmx = 0;
	seenlocal = 0;
	buflen = sizeof(hostbuf);
	bp = hostbuf;
	ancount = ntohs(hp->ancount);
	while (--ancount >= 0 && cp < eom && nmx < MAXMXHOSTS) {
		if ((n = dn_expand((u_char *)&answer,
		    eom, cp, (u_char *)bp, buflen)) < 0)
			break;
		cp += n;
		GETSHORT(type, cp);
 		cp += sizeof(u_short) + sizeof(u_long);
		GETSHORT(n, cp);
		if (type != T_MX)  {
			if (tTd(8, 1) || _res.options & RES_DEBUG)
				printf("unexpected answer type %d, size %d\n",
				    type, n);
			cp += n;
			continue;
		}
		GETSHORT(pref, cp);
		if ((n = dn_expand((u_char *)&answer,
		    eom, cp, (u_char *)bp, buflen)) < 0)
			break;
		cp += n;
		if (!strcasecmp(bp, localhost)) {
			if (seenlocal == 0 || pref < localpref)
				localpref = pref;
			seenlocal = 1;
			continue;
		}
		prefer[nmx] = pref;
		mxhosts[nmx++] = bp;
		n = strlen(bp) + 1;
		bp += n;
		buflen -= n;
	}
	if (nmx == 0) {
punt:		mxhosts[0] = strcpy(hostbuf, host);
		return(1);
	}

	/* sort the records */
	for (i = 0; i < nmx; i++) {
		for (j = i + 1; j < nmx; j++) {
			if (prefer[i] > prefer[j] ||
			    (prefer[i] == prefer[j] && (rand() & 0100) == 0)) {
				register int temp;
				register char *temp1;

				temp = prefer[i];
				prefer[i] = prefer[j];
				prefer[j] = temp;
				temp1 = mxhosts[i];
				mxhosts[i] = mxhosts[j];
				mxhosts[j] = temp1;
			}
		}
		if (seenlocal && prefer[i] >= localpref) {
			/*
			 * truncate higher pref part of list; if we're
			 * the best choice left, we should have realized
			 * awhile ago that this was a local delivery.
			 */
			if (i == 0) {
				*rcode = EX_CONFIG;
				return(-1);
			}
			nmx = i;
			break;
		}
	}
	return(nmx);
}

/*
 * Use query type of ANY if possible (NoWildcardMX), which will
 * find types CNAME, A, and MX, and will cause all existing records
 * to be cached by our local server.  If there is (might be) a
 * wildcard MX record in the local domain or its parents that are
 * searched, we can't use ANY; it would cause fully-qualified names
 * to match as names in a local domain.
 */

bool
getcanonname(host, hbsize)
	char *host;
	int hbsize;
{
	extern int h_errno;
	register u_char *eom, *ap;
	register char *cp;
	register int n; 
	HEADER *hp;
	querybuf answer;
	int first, ancount, qdcount, loopcnt;
	int ret;
	int qtype = NoWildcardMX ? T_ANY : T_CNAME;
	char **domain;
	bool rval;
	int type;
	char nbuf[PACKETSZ];

	if (tTd(8, 2))
		printf("getcanonname(%s)\n", host);

	if ((_res.options & RES_INIT) == 0 && res_init() == -1)
		return (FALSE);

	loopcnt = 0;
	rval = FALSE;
loop:
	for (cp = host, n = 0; *cp; cp++)
		if (*cp == '.')
			n++;
	if (n > 0 && *--cp == '.')
	{
		cp = host;
		n = -1;
	}

	/*
	 * We do at least one level of search if
	 *	- there is no dot and RES_DEFNAME is set, or
	 *	- there is at least one dot, there is no trailing dot,
	 *	  and RES_DNSRCH is set.
	 */
	ret = -1;
	if ((n == 0 && _res.options & RES_DEFNAMES) ||
	   (n > 0 && *--cp != '.' && _res.options & RES_DNSRCH))
	{
		for (domain = _res.dnsrch; *domain; domain++)
		{
			(void) sprintf(nbuf, "%.*s.%.*s",
				MAXDNAME, host, MAXDNAME, *domain);
			if (tTd(8, 5))
				printf("getcanonname: trying %s\n", nbuf);
			ret = res_query(nbuf, C_IN, qtype, &answer, sizeof(answer));
			if (ret > 0)
			{
				if (tTd(8, 8))
					printf("\tYES\n");
				cp = nbuf;
				break;
			}
			else if (tTd(8, 8))
				printf("\tNO: h_errno=%d\n", h_errno);

			/*
			 * If no server present, give up.
			 * If name isn't found in this domain,
			 * keep trying higher domains in the search list
			 * (if that's enabled).
			 * On a NO_DATA error, keep trying, otherwise
			 * a wildcard entry of another type could keep us
			 * from finding this entry higher in the domain.
			 * If we get some other error (negative answer or
			 * server failure), then stop searching up,
			 * but try the input name below in case it's fully-qualified.
			 */
			if (errno == ECONNREFUSED) {
				h_errno = TRY_AGAIN;
				return FALSE;
			}
			if (h_errno == NO_DATA)
			{
				ret = 0;
				cp = nbuf;
				break;
			}
			if ((h_errno != HOST_NOT_FOUND) ||
			    (_res.options & RES_DNSRCH) == 0)
				return FALSE;
		}
	}
	if (ret < 0)
	{
		/*
		**  Try the unmodified name.
		*/

		cp = host;
		if (tTd(8, 5))
			printf("getcanonname: trying %s\n", cp);
		ret = res_query(cp, C_IN, qtype, &answer, sizeof(answer));
		if (ret > 0)
		{
			if (tTd(8, 8))
				printf("\tYES\n");
		}
		else
		{
			if (tTd(8, 8))
				printf("\tNO: h_errno=%d\n", h_errno);
			if (h_errno != NO_DATA)
				return FALSE;
		}
	}

	/* find first satisfactory answer */
	hp = (HEADER *)&answer;
	ancount = ntohs(hp->ancount);
	if (tTd(8, 3))
		printf("rcode = %d, ancount=%d, qdcount=%d\n",
			hp->rcode, ancount, ntohs(hp->qdcount));

	/* we don't care about errors here, only if we got an answer */
	if (ancount == 0)
	{
		strncpy(host, cp, hbsize);
		host[hbsize - 1] = '\0';
		return (TRUE);
	}
	ap = (u_char *)&answer + sizeof(HEADER);
	eom = (u_char *)&answer + ret;
	for (qdcount = ntohs(hp->qdcount); qdcount--; ap += ret + QFIXEDSZ)
	{
		if ((ret = dn_skipname(ap, eom)) < 0)
		{
			if (tTd(8, 20))
				printf("qdcount failure (%d)\n",
					ntohs(hp->qdcount));
			return FALSE;		/* ???XXX??? */
		}
	}

	/*
	* just in case someone puts a CNAME record after another record,
	* check all records for CNAME; otherwise, just take the first
	* name found.
	*/
	for (first = 1; --ancount >= 0 && ap < eom; ap += ret)
	{
		if ((ret = dn_expand((u_char *)&answer,
		    eom, ap, (u_char *)nbuf, sizeof(nbuf))) < 0)
			break;
		if (first) {			/* XXX */
			(void)strncpy(host, nbuf, hbsize);
			host[hbsize - 1] = '\0';
			first = 0;
			rval = TRUE;
		}
		ap += ret;
		GETSHORT(type, ap);
		ap += sizeof(u_short) + sizeof(u_long);
		GETSHORT(ret, ap);
		if (type == T_CNAME)  {
			/*
			 * assume that only one cname will be found.  More
			 * than one is undefined.  Copy so that if dn_expand
			 * fails, `host' is still okay.
			 */
			if ((ret = dn_expand((u_char *)&answer,
			    eom, ap, (u_char *)nbuf, sizeof(nbuf))) < 0)
				break;
			(void)strncpy(host, nbuf, hbsize); /* XXX */
			host[hbsize - 1] = '\0';
			if (++loopcnt > 8)	/* never be more than 1 */
				return FALSE;
			rval = TRUE;
			goto loop;
		}
	}
	return rval;		/* ???XXX??? */
}

#else /* not NAMED_BIND */

#include <netdb.h>

bool
getcanonname(host, hbsize)
	char *host;
	int hbsize;
{
	struct hostent *hp;

	hp = gethostbyname(host);
	if (hp == NULL)
		return (FALSE);

	if (strlen(hp->h_name) >= hbsize)
		return (FALSE);

	(void) strcpy(host, hp->h_name);
	return (TRUE);
}

#endif /* not NAMED_BIND */
