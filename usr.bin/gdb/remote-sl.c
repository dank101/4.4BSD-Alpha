/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Van Jacobson and Steven McCanne of Lawrence Berkeley Laboratory.
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
 *
 * $Header: remote-sl.c,v 1.2 92/07/23 19:38:15 mccanne Exp $ (LBL);
 */

#ifndef lint
static char sccsid[] = "@(#)remote-sl.c	6.6 (Berkeley) 7/24/92";
#endif /* not lint */

/*
 * XXX These come from remote.h in gdb-4.x.  Here, remote.c just
 * uses the truth value of the error.
 * Error codes.
 */
#define EKGDB_CSUM      1       /* failed checksum */
#define EKGDB_2BIG      2       /* "giant" packet */
#define EKGDB_RUNT      3       /* short packet */
#define EKGDB_BADOP     4       /* bad op code in packet */
#define EKGDB_TIMEOUT   5       /* request timed out */
#define EKGDB_IO        6       /* generic I/O error */

#include "param.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/time.h>

#include <errno.h>
extern int errno;

#ifdef USG
#include <fcntl.h>
#endif

#ifdef HAVE_TERMIO
#include <termio.h>
#undef TIOCGETP
#define TIOCGETP TCGETA
#undef TIOCSETN
#define TIOCSETN TCSETA
#undef TIOCSETP
#define TIOCSETP TCSETAF
#define TERMINAL struct termio
#else
#include <sgtty.h>
#define TERMINAL struct sgttyb
#endif

#ifndef FD_SETSIZE
#define FD_SET(n, fdp) ((fdp)->fds_bits[0] |= (1 << (n)))
#define FD_ISSET(n, fdp) ((fdp)->fds_bits[0] & (1 << (n)))
#define FD_ZERO(fdp) ((fdp)->fds_bits[0] = 0)
#endif

#include "defs.h"
#include "remote-sl.h"

static int sl_send();
static int sl_recv();

/*
 * Descriptor for I/O to remote machine.
 */
static int sl_fd = -1;

/*
 * User-configurable baud rate of serial line.
 */
static int speed = B9600;

/*
 * Statistics.
 */
static int sl_errs;
static int sl_inbytes;
static int sl_outbytes;;

static void
sl_close()
{
	if (sl_fd >= 0) {
		close(sl_fd);
		sl_fd = -1;
	}
}

/*
 * Open a serial line for remote debugging.
 */
int
sl_open(name, send_fn, recv_fn, close_fn, mtu, bufsize)
	char *name;
	int (**send_fn)();
	int (**recv_fn)();
	void (**close_fn)();
	int *mtu;
	int *bufsize;
{
	TERMINAL sg;
	char device[80];

	if (sl_fd >= 0)
		close(sl_fd);

	if (name[0] != '/') {
		sprintf(device, "/dev/%s", name);
		name = device;
	}
	sl_fd = open(name, O_RDWR);
	if (sl_fd < 0)
		perror_with_name(name);

	ioctl(sl_fd, TIOCGETP, &sg);
#ifdef HAVE_TERMIO
	sg.c_lflag &= ~(ICANON | ECHO);
	/* Set speed, 8 bit characters, enable receiver, non-dialup. */
	tios.c_cflag = (speed & CBAUD)|CS8|CREAD|CLOCAL;
#else
	sg.sg_flags = RAW | ANYP;
	sg.sg_ispeed = sg.sg_ospeed = speed;
#endif
	ioctl(sl_fd, TIOCSETP, &sg);

	*send_fn = sl_send;
	*recv_fn = sl_recv;
	*close_fn = sl_close;
	
	/*
	 * The mtu of the remote level is the maximal message the length.
	 * The buffer size is one character larger so we can stuff in the
	 * checksum without special casing.
	 */
	*mtu = SL_MAXDATA;
	*bufsize = SL_RPCSIZE;

	sl_errs = 0;
	sl_inbytes = 0;
	sl_outbytes = 0;

	return 0;
}

/*
 * Remote input buffering.
 */
static u_char rib[2 * SL_MTU];
static u_char *rib_cp, *rib_ep;
#define GETC ((rib_cp < rib_ep) ? *rib_cp++ : rib_filbuf())

static int
rib_filbuf()
{
	int cc, fd = sl_fd;
	fd_set fds;

	static struct timeval timeout = { 5L, 0L };

	FD_ZERO(&fds);
	while (1) {
		FD_SET(fd, &fds);
		cc = select(fd + 1, &fds, (fd_set *)0, (fd_set *)0, &timeout);
		if (cc > 0) {
			cc = read(fd, (char *)rib, sizeof rib);
			if (cc < 0) {
				print_sys_errmsg("remote read", errno);
				continue;
			}
			rib_cp = &rib[1];
			rib_ep = &rib[cc];

			sl_inbytes += cc;

			return rib[0];
		}
		if (cc < 0)
			print_sys_errmsg("remote readchar select failed", 
					 errno);
	}
}

#define PUTESC(p, c) { \
	if (c == FRAME_END) { \
		*p++ = FRAME_ESCAPE; \
		c = TRANS_FRAME_END; \
	} else if (c == FRAME_ESCAPE) { \
		*p++ = FRAME_ESCAPE; \
		c = TRANS_FRAME_ESCAPE; \
	} else if (c == FRAME_START) { \
		*p++ = FRAME_ESCAPE; \
		c = TRANS_FRAME_START; \
	} \
	*p++ = c; \
}

/*
 * Send a message to the remote host.  An error code is returned.
 */
static int
sl_send(type, bp, len)
	register u_char type;
	register u_char *bp;
	register int len;
{
	register u_char *p, *ep;
	register u_char csum, c;
	u_char buf[SL_MTU];

	/*
	 * Build a packet.  The framing byte comes first, then the command
	 * byte, the message, the checksum, and another framing character.
	 * We must escape any bytes that match the framing or escape chars.
	 */
	p = buf;
	*p++ = FRAME_START;
	csum = type;
	PUTESC(p, type);

	for (ep = bp + len; bp < ep; ) {
		c = *bp++;
		csum += c;
		PUTESC(p, c);
	}
	csum = -csum;
	PUTESC(p, csum);
	*p++ = FRAME_END;

	len = p - buf;
	sl_outbytes += len;
	if (write(sl_fd, (caddr_t)buf, len) != len)
		return (EKGDB_IO);
	return (0);
}

/*
 * Read a packet from the remote machine.  An error code is returned.
 */
static int
sl_recv(tp, ip, lenp)
	int *tp;
	u_char *ip;
	int *lenp;
{
	register u_char csum, *bp;
	register int c;
	register int escape, len;
	register int type;
	u_char buf[SL_RPCSIZE + 1];	/* room for checksum at end of buffer */

	/*
	 * Allow immediate quit while reading from device, it could be hung.
	 */
	++immediate_quit;

	/*
	 * Throw away garbage characters until we see the start
	 * of a frame (i.e., don't let framing errors propagate up).
	 * If we don't do this, we can get pretty confused.
	 */
	while ((c = GETC) != FRAME_START)
		if (c < 0)
			return (-c);
restart:
	csum = len = escape = 0;
	type = -1;
	bp = buf;
	while (1) {
		c = GETC;
		if (c < 0)
			return (-c);

		switch (c) {
			
		case FRAME_ESCAPE:
			escape = 1;
			continue;
			
		case TRANS_FRAME_ESCAPE:
			if (escape)
				c = FRAME_ESCAPE;
			break;
			
		case TRANS_FRAME_END:
			if (escape)
				c = FRAME_END;
			break;

		case TRANS_FRAME_START:
			if (escape)
				c = FRAME_START;
			break;

		case FRAME_START:
			goto restart;
			
		case FRAME_END:
			if (type < 0 || --len < 0) {
				csum = len = escape = 0;
				continue;
			}
			if (csum != 0) {
				++sl_errs;
				return (EKGDB_CSUM);
			}
			--immediate_quit;

			/* Store saved rpc reply type */
			*tp = type;

			/* Store length of rpc reply packet */
			if (lenp)
				*lenp = len;

			if (ip)
				bcopy((caddr_t)buf, (caddr_t)ip, len);
			return (0);
		}
		csum += c;
		if (type < 0) {
			type = c;
			escape = 0;
			continue;
		}
		if (++len > sizeof(buf)) {
			do {
				if ((c = GETC) < 0)
					return (-c);
			} while (c != FRAME_END);

			return (EKGDB_2BIG);
		}
		*bp++ = c;

		escape = 0;
	}
}

static void
set_sl_baudrate_command(arg, from_tty)
	char           *arg;
	int             from_tty;
{
	int baudrate;

	if (arg == 0)
		error_no_arg("set remote-baudrate");
	while (*arg == ' ' || *arg == '\t')
		++arg;
	if (*arg == 0)
		error_no_arg("set remote-baudrate");
	if (*arg < '0' || *arg > '9')
		error("non-numeric arg to \"set remote-baudrate\".");

	baudrate = atoi(arg);
	switch (baudrate) {
	case 0:		speed = B0; break;
	case 50:	speed = B50; break;
	case 75:	speed = B75; break;
	case 110:	speed = B110; break;
	case 134:	speed = B134; break;
	case 150:	speed = B150; break;
	case 200:	speed = B200; break;
	case 300:	speed = B300; break;
	case 600:	speed = B600; break;
	case 1200:	speed = B1200; break;
	case 1800:	speed = B1800; break;
	case 2400:	speed = B2400; break;
	case 4800:	speed = B4800; break;
	case 9600:	speed = B9600; break;
	case 19200:	speed = EXTA; break;
	case 38400:	speed = EXTB; break;
	default:
		error("unknown baudrate for \"set remote-baudrate\".");
	}
}

/* ARGSUSED */
static void
sl_info(arg, from_tty)
	char *arg;
	int from_tty;
{
	int linespeed;

	switch (speed) {
	default:	linespeed = 0; break;
	case B50:	linespeed = 50; break;
	case B75:	linespeed = 75; break;
	case B110:	linespeed = 110; break;
	case B134:	linespeed = 134; break;
	case B150:	linespeed = 150; break;
	case B200:	linespeed = 200; break;
	case B300:	linespeed = 300; break;
	case B600:	linespeed = 600; break;
	case B1200:	linespeed = 1200; break;
	case B1800:	linespeed = 1800; break;
	case B2400:	linespeed = 2400; break;
	case B4800:	linespeed = 4800; break;
	case B9600:	linespeed = 9600; break;
	case EXTA:	linespeed = 19200; break;
	case EXTB:	linespeed = 38400; break;
	}
	printf("sl-baudrate     %6d\n", linespeed);
	printf("bytes received  %6d\n", sl_inbytes);
	printf("bytes sent      %6d\n", sl_outbytes);
	printf("checksum errors %6d\n", sl_errs);
}

extern struct cmd_list_element *setlist;

void
_initialize_sl()
{
	add_info("sl", sl_info,
		 "Show current settings of serial line debugging options.");
	add_cmd("sl-baudrate", class_support, set_sl_baudrate_command,
		"Set remote debug serial line baudrate.", &setlist);
}
