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
static char sccsid[] = "@(#)recipient.c	5.35 (Berkeley) 7/19/92";
#endif /* not lint */

# include <sys/types.h>
# include <sys/stat.h>
# include <sys/file.h>
# include <pwd.h>
# include "sendmail.h"

/*
**  SENDTOLIST -- Designate a send list.
**
**	The parameter is a comma-separated list of people to send to.
**	This routine arranges to send to all of them.
**
**	Parameters:
**		list -- the send list.
**		ctladdr -- the address template for the person to
**			send to -- effective uid/gid are important.
**			This is typically the alias that caused this
**			expansion.
**		sendq -- a pointer to the head of a queue to put
**			these people into.
**
**	Returns:
**		none
**
**	Side Effects:
**		none.
*/

# define MAXRCRSN	10

sendtolist(list, ctladdr, sendq, e)
	char *list;
	ADDRESS *ctladdr;
	ADDRESS **sendq;
	register ENVELOPE *e;
{
	register char *p;
	register ADDRESS *al;	/* list of addresses to send to */
	bool firstone;		/* set on first address sent */
	bool selfref;		/* set if this list includes ctladdr */
	char delimiter;		/* the address delimiter */

	if (tTd(25, 1))
	{
		printf("sendto: %s\n   ctladdr=", list);
		printaddr(ctladdr, FALSE);
	}

	/* heuristic to determine old versus new style addresses */
	if (ctladdr == NULL &&
	    (index(list, ',') != NULL || index(list, ';') != NULL ||
	     index(list, '<') != NULL || index(list, '(') != NULL))
		e->e_flags &= ~EF_OLDSTYLE;
	delimiter = ' ';
	if (!bitset(EF_OLDSTYLE, e->e_flags) || ctladdr != NULL)
		delimiter = ',';

	firstone = TRUE;
	selfref = FALSE;
	al = NULL;

	for (p = list; *p != '\0'; )
	{
		register ADDRESS *a;
		extern char *DelimChar;		/* defined in prescan */

		/* parse the address */
		while (isspace(*p) || *p == ',')
			p++;
		a = parseaddr(p, (ADDRESS *) NULL, 1, delimiter, e);
		p = DelimChar;
		if (a == NULL)
			continue;
		a->q_next = al;
		a->q_alias = ctladdr;

		/* see if this should be marked as a primary address */
		if (ctladdr == NULL ||
		    (firstone && *p == '\0' && bitset(QPRIMARY, ctladdr->q_flags)))
			a->q_flags |= QPRIMARY;

		/* put on send queue or suppress self-reference */
		if (ctladdr != NULL && sameaddr(ctladdr, a))
			selfref = TRUE;
		else
			al = a;
		firstone = FALSE;
	}

	/* if this alias doesn't include itself, delete ctladdr */
	if (!selfref && ctladdr != NULL)
		ctladdr->q_flags |= QDONTSEND;

	/* arrange to send to everyone on the local send list */
	while (al != NULL)
	{
		register ADDRESS *a = al;
		extern ADDRESS *recipient();

		al = a->q_next;
		a = recipient(a, sendq, e);

		/* arrange to inherit full name */
		if (a->q_fullname == NULL && ctladdr != NULL)
			a->q_fullname = ctladdr->q_fullname;
	}

	e->e_to = NULL;
}
/*
**  RECIPIENT -- Designate a message recipient
**
**	Saves the named person for future mailing.
**
**	Parameters:
**		a -- the (preparsed) address header for the recipient.
**		sendq -- a pointer to the head of a queue to put the
**			recipient in.  Duplicate supression is done
**			in this queue.
**
**	Returns:
**		The actual address in the queue.  This will be "a" if
**		the address is not a duplicate, else the original address.
**
**	Side Effects:
**		none.
*/

extern ADDRESS *getctladdr();
extern char	*RcptLogFile;

ADDRESS *
recipient(a, sendq, e)
	register ADDRESS *a;
	register ADDRESS **sendq;
	register ENVELOPE *e;
{
	register ADDRESS *q;
	ADDRESS **pq;
	register struct mailer *m;
	register char *p;
	bool quoted = FALSE;		/* set if the addr has a quote bit */
	int findusercount = 0;
	char buf[MAXNAME];		/* unquoted image of the user name */
	extern bool safefile();

	e->e_to = a->q_paddr;
	m = a->q_mailer;
	errno = 0;
	if (tTd(26, 1))
	{
		printf("\nrecipient: ");
		printaddr(a, FALSE);
	}

	/* break aliasing loops */
	if (AliasLevel > MAXRCRSN)
	{
		usrerr("aliasing/forwarding loop broken");
		return (a);
	}

	/*
	**  Finish setting up address structure.
	*/

	/* set the queue timeout */
	a->q_timeout = TimeOut;

	/* map user & host to lower case if requested on non-aliases */
	if (a->q_alias == NULL)
		loweraddr(a);

	/* get unquoted user for file, program or user.name check */
	(void) strcpy(buf, a->q_user);
	for (p = buf; *p != '\0' && !quoted; p++)
	{
		if (*p == '\\')
			quoted = TRUE;
	}
	stripquotes(buf);

	/* do sickly crude mapping for program mailing, etc. */
	if (m == LocalMailer && buf[0] == '|')
	{
		a->q_mailer = m = ProgMailer;
		a->q_user++;
		if (a->q_alias == NULL && !ForceMail)
		{
			a->q_flags |= QDONTSEND|QBADADDR;
			usrerr("Cannot mail directly to programs");
		}
	}

	/*
	**  Look up this person in the recipient list.
	**	If they are there already, return, otherwise continue.
	**	If the list is empty, just add it.  Notice the cute
	**	hack to make from addresses suppress things correctly:
	**	the QDONTSEND bit will be set in the send list.
	**	[Please note: the emphasis is on "hack."]
	*/

	for (pq = sendq; (q = *pq) != NULL; pq = &q->q_next)
	{
		if (!ForceMail && sameaddr(q, a))
		{
			if (tTd(26, 1))
			{
				printf("%s in sendq: ", a->q_paddr);
				printaddr(q, FALSE);
			}
			if (!bitset(QDONTSEND, a->q_flags))
				message(Arpa_Info, "duplicate suppressed");
			if (!bitset(QPRIMARY, q->q_flags))
				q->q_flags |= a->q_flags;
			return (q);
		}
	}

	/* add address on list */
	*pq = a;
	a->q_next = NULL;
	e->e_nrcpts++;

	if (a->q_alias == NULL && RcptLogFile != NULL &&
	    !bitset(QDONTSEND, a->q_flags))
	{
		static int RcptLogFd = -1;

		/*
		**  Log the incoming recipient name before aliasing,
		**  expanding, forwarding, rewriting, and all that jazz.
		**  We'll use this to track down out-of-date aliases,
		**  host names, and so forth.
		*/

		if (RcptLogFd < 0)
		{
			/* try to open the log file */
			RcptLogFd = open(RcptLogFile, O_WRONLY|O_APPEND|O_CREAT, 0666);
			if (RcptLogFd >= 0)
				(void) fcntl(RcptLogFd, F_SETFD, 1);
		}
		if (RcptLogFd >= 0)
		{
			int l = strlen(a->q_paddr);

			a->q_paddr[l] = '\n';
			if (write(RcptLogFd, a->q_paddr, l + 1) < 0)
			{
				(void) close(RcptLogFd);
				RcptLogFd = -1;
			}
			a->q_paddr[l] = '\0';
		}
	}

	/*
	**  Alias the name and handle :include: specs.
	*/

  trylocaluser:
	if (tTd(29, 7))
		printf("at trylocaluser %s\n", a->q_user);

	if (m == LocalMailer && !bitset(QDONTSEND, a->q_flags))
	{
		if (strncmp(a->q_user, ":include:", 9) == 0)
		{
			a->q_flags |= QDONTSEND;
			if (a->q_alias == NULL && !ForceMail)
			{
				a->q_flags |= QBADADDR;
				usrerr("Cannot mail directly to :include:s");
			}
			else
			{
				message(Arpa_Info, "including file %s", &a->q_user[9]);
				include(&a->q_user[9], FALSE, a, sendq, e);
			}
		}
		else
		{
			/* try aliasing */
			alias(a, sendq, e);

# ifdef USERDB
			/* if not aliased, look it up in the user database */
			if (!bitset(QDONTSEND|QNOTREMOTE, a->q_flags))
			{
				extern int udbexpand();

				if (udbexpand(a, sendq, e) == EX_TEMPFAIL)
				{
					a->q_flags |= QQUEUEUP;
					if (e->e_message == NULL)
						e->e_message = newstr("Deferred: user database error");
# ifdef LOG
					if (LogLevel > 3)
						syslog(LOG_INFO, "%s: deferred: udbexpand",
							e->e_id);
# endif
					message(Arpa_Info, "queued (user database error)");
					return (a);
				}
			}
# endif
		}
	}

	/*
	**  If the user is local and still being sent, verify that
	**  the address is good.  If it is, try to forward.
	**  If the address is already good, we have a forwarding
	**  loop.  This can be broken by just sending directly to
	**  the user (which is probably correct anyway).
	*/

	if (bitset(QDONTSEND, a->q_flags) || m != LocalMailer)
		return (a);

	/* see if this is to a file */
	if (buf[0] == '/')
	{
		struct stat stb;
		extern bool writable();

		p = rindex(buf, '/');
		/* check if writable or creatable */
		if (a->q_alias == NULL && !QueueRun && !ForceMail)
		{
			a->q_flags |= QDONTSEND|QBADADDR;
			usrerr("Cannot mail directly to files");
		}
		else if ((stat(buf, &stb) >= 0) ? (!writable(&stb)) :
		    (*p = '\0', !safefile(buf, getruid(), S_IWRITE|S_IEXEC)))
		{
			a->q_flags |= QBADADDR;
			giveresponse(EX_CANTCREAT, m, e);
		}
		return (a);
	}

	/*
	**  If we have a level two config file, then pass the name through
	**  Ruleset 5 before sending it off.  Ruleset 5 has the right
	**  to send rewrite it to another mailer.  This gives us a hook
	**  after local aliasing has been done.
	*/

	if (tTd(29, 5))
	{
		printf("recipient: testing local?  cl=%d, rr5=%x\n\t",
			ConfigLevel, RewriteRules[5]);
		printaddr(a, FALSE);
	}
	if (!bitset(QNOTREMOTE, a->q_flags) && ConfigLevel >= 2 &&
	    RewriteRules[5] != NULL)
	{
		maplocaluser(a, sendq, e);
	}

	/*
	**  If it didn't get rewritten to another mailer, go ahead
	**  and deliver it.
	*/

	if (!bitset(QDONTSEND, a->q_flags))
	{
		auto bool fuzzy;
		register struct passwd *pw;
		extern struct passwd *finduser();

		/* warning -- finduser may trash buf */
		pw = finduser(buf, &fuzzy);
		if (pw == NULL)
		{
			a->q_flags |= QBADADDR;
			giveresponse(EX_NOUSER, m, e);
		}
		else
		{
			char nbuf[MAXNAME];

			if (fuzzy)
			{
				/* name was a fuzzy match */
				a->q_user = newstr(pw->pw_name);
				if (findusercount++ > 3)
				{
					usrerr("aliasing/forwarding loop for %s broken",
						pw->pw_name);
					return (a);
				}

				/* see if it aliases */
				(void) strcpy(buf, pw->pw_name);
				goto trylocaluser;
			}
			a->q_home = newstr(pw->pw_dir);
			a->q_uid = pw->pw_uid;
			a->q_gid = pw->pw_gid;
			a->q_flags |= QGOODUID;
			buildfname(pw->pw_gecos, pw->pw_name, nbuf);
			if (nbuf[0] != '\0')
				a->q_fullname = newstr(nbuf);
			if (!quoted)
				forward(a, sendq, e);
		}
	}
	return (a);
}
/*
**  FINDUSER -- find the password entry for a user.
**
**	This looks a lot like getpwnam, except that it may want to
**	do some fancier pattern matching in /etc/passwd.
**
**	This routine contains most of the time of many sendmail runs.
**	It deserves to be optimized.
**
**	Parameters:
**		name -- the name to match against.
**		fuzzyp -- an outarg that is set to TRUE if this entry
**			was found using the fuzzy matching algorithm;
**			set to FALSE otherwise.
**
**	Returns:
**		A pointer to a pw struct.
**		NULL if name is unknown or ambiguous.
**
**	Side Effects:
**		may modify name.
*/

struct passwd *
finduser(name, fuzzyp)
	char *name;
	bool *fuzzyp;
{
	register struct passwd *pw;
	register char *p;
	extern struct passwd *getpwent();
	extern struct passwd *getpwnam();

	if (tTd(29, 4))
		printf("finduser(%s): ", name);

	/* map upper => lower case */
	for (p = name; *p != '\0'; p++)
	{
		if (isascii(*p) && isupper(*p))
			*p = tolower(*p);
	}
	*fuzzyp = FALSE;

	/* look up this login name using fast path */
	if ((pw = getpwnam(name)) != NULL)
	{
		if (tTd(29, 4))
			printf("found (non-fuzzy)\n");
		return (pw);
	}

#ifdef MATCHGECOS
	/* see if fuzzy matching allowed */
	if (!MatchGecos)
	{
		if (tTd(29, 4))
			printf("not found (fuzzy disabled)\n");
		return NULL;
	}

	/* search for a matching full name instead */
	for (p = name; *p != '\0'; p++)
	{
		if (*p == (SpaceSub & 0177) || *p == '_')
			*p = ' ';
	}
	(void) setpwent();
	while ((pw = getpwent()) != NULL)
	{
		char buf[MAXNAME];

		buildfname(pw->pw_gecos, pw->pw_name, buf);
		if (index(buf, ' ') != NULL && !strcasecmp(buf, name))
		{
			if (tTd(29, 4))
				printf("fuzzy matches %s\n", pw->pw_name);
			message(Arpa_Info, "sending to login name %s", pw->pw_name);
			*fuzzyp = TRUE;
			return (pw);
		}
	}
#endif
	if (tTd(29, 4))
		printf("no fuzzy match found\n");
	return (NULL);
}
/*
**  WRITABLE -- predicate returning if the file is writable.
**
**	This routine must duplicate the algorithm in sys/fio.c.
**	Unfortunately, we cannot use the access call since we
**	won't necessarily be the real uid when we try to
**	actually open the file.
**
**	Notice that ANY file with ANY execute bit is automatically
**	not writable.  This is also enforced by mailfile.
**
**	Parameters:
**		s -- pointer to a stat struct for the file.
**
**	Returns:
**		TRUE -- if we will be able to write this file.
**		FALSE -- if we cannot write this file.
**
**	Side Effects:
**		none.
*/

bool
writable(s)
	register struct stat *s;
{
	uid_t euid;
	gid_t egid;
	int bits;

	if (bitset(0111, s->st_mode))
		return (FALSE);
	euid = getruid();
	egid = getrgid();
	if (geteuid() == 0)
	{
		if (bitset(S_ISUID, s->st_mode))
			euid = s->st_uid;
		if (bitset(S_ISGID, s->st_mode))
			egid = s->st_gid;
	}

	if (euid == 0)
		return (TRUE);
	bits = S_IWRITE;
	if (euid != s->st_uid)
	{
		bits >>= 3;
		if (egid != s->st_gid)
			bits >>= 3;
	}
	return ((s->st_mode & bits) != 0);
}
/*
**  INCLUDE -- handle :include: specification.
**
**	Parameters:
**		fname -- filename to include.
**		forwarding -- if TRUE, we are reading a .forward file.
**			if FALSE, it's a :include: file.
**		ctladdr -- address template to use to fill in these
**			addresses -- effective user/group id are
**			the important things.
**		sendq -- a pointer to the head of the send queue
**			to put these addresses in.
**
**	Returns:
**		none.
**
**	Side Effects:
**		reads the :include: file and sends to everyone
**		listed in that file.
*/

static jmp_buf	CtxIncludeTimeout;

include(fname, forwarding, ctladdr, sendq, e)
	char *fname;
	bool forwarding;
	ADDRESS *ctladdr;
	ADDRESS **sendq;
	ENVELOPE *e;
{
	register FILE *fp;
	char *oldto = e->e_to;
	char *oldfilename = FileName;
	int oldlinenumber = LineNumber;
	register EVENT *ev = NULL;
	char buf[MAXLINE];
	static int includetimeout();

	/*
	**  If home directory is remote mounted but server is down,
	**  this can hang or give errors; use a timeout to avoid this
	*/

	if (setjmp(CtxIncludeTimeout) != 0)
	{
		ctladdr->q_flags |= QQUEUEUP|QDONTSEND;
		errno = 0;
		usrerr("451 open timeout on %s", fname);
		return;
	}
	ev = setevent((time_t) 60, includetimeout, 0);

	/* if forwarding, the input file must be marked safe */
	if (forwarding && !safefile(fname, ctladdr->q_uid, S_IREAD))
	{
		/* don't use this .forward file */
		clrevent(ev);
		return;
	}

	fp = fopen(fname, "r");
	if (fp == NULL)
	{
		usrerr("Cannot open %s", fname);
		return;
	}

	if (getctladdr(ctladdr) == NULL)
	{
		struct stat st;

		if (fstat(fileno(fp), &st) < 0)
			syserr("Cannot fstat %s!", fname);
		ctladdr->q_uid = st.st_uid;
		ctladdr->q_gid = st.st_gid;
		ctladdr->q_flags |= QGOODUID;
	}

	clrevent(ev);

	/* read the file -- each line is a comma-separated list. */
	FileName = fname;
	LineNumber = 0;
	while (fgets(buf, sizeof buf, fp) != NULL)
	{
		register char *p = index(buf, '\n');

		LineNumber++;
		if (p != NULL)
			*p = '\0';
		if (buf[0] == '\0' || buf[0] == '#')
			continue;
		e->e_to = oldto;
		message(Arpa_Info, "%s to %s",
			forwarding ? "forwarding" : "sending", buf);
		AliasLevel++;
		sendtolist(buf, ctladdr, sendq, e);
		AliasLevel--;
	}

	(void) fclose(fp);
	FileName = oldfilename;
	LineNumber = oldlinenumber;
}

static
includetimeout()
{
	longjmp(CtxIncludeTimeout, 1);
}
/*
**  SENDTOARGV -- send to an argument vector.
**
**	Parameters:
**		argv -- argument vector to send to.
**
**	Returns:
**		none.
**
**	Side Effects:
**		puts all addresses on the argument vector onto the
**			send queue.
*/

sendtoargv(argv, e)
	register char **argv;
	register ENVELOPE *e;
{
	register char *p;

	while ((p = *argv++) != NULL)
	{
		if (argv[0] != NULL && argv[1] != NULL && !strcasecmp(argv[0], "at"))
		{
			char nbuf[MAXNAME];

			if (strlen(p) + strlen(argv[1]) + 2 > sizeof nbuf)
				usrerr("address overflow");
			else
			{
				(void) strcpy(nbuf, p);
				(void) strcat(nbuf, "@");
				(void) strcat(nbuf, argv[1]);
				p = newstr(nbuf);
				argv += 2;
			}
		}
		sendtolist(p, (ADDRESS *) NULL, &e->e_sendqueue, e);
	}
}
/*
**  GETCTLADDR -- get controlling address from an address header.
**
**	If none, get one corresponding to the effective userid.
**
**	Parameters:
**		a -- the address to find the controller of.
**
**	Returns:
**		the controlling address.
**
**	Side Effects:
**		none.
*/

ADDRESS *
getctladdr(a)
	register ADDRESS *a;
{
	while (a != NULL && !bitset(QGOODUID, a->q_flags))
		a = a->q_alias;
	return (a);
}
