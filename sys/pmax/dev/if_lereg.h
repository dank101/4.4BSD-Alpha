/*
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	@(#)if_lereg.h	7.2 (Berkeley) 2/29/92
 */

#define	LEMTU		1518
#define	LEMINSIZE	60	/* should be 64 if mode DTCR is set */
#define	LERBUF		32
#define	LERBUFLOG2	5
#define	LE_RLEN		(LERBUFLOG2 << 13)
#define	LETBUF		8
#define	LETBUFLOG2	3
#define	LE_TLEN		(LETBUFLOG2 << 13)

/*
 * LANCE registers.
 */
struct lereg1 {
	u_short	ler1_rdp;	/* data port */
	short	pad0;
	u_short	ler1_rap;	/* register select port */
	short	pad1;
};

#ifdef DS3100
#define LEPAD(x)	short x;
#define LE_RAM_SIZE	0x10000

typedef u_short	le_buf_t;
#endif
#ifdef DS5000
#define LEPAD(x)
#define LE_RAM_SIZE	0x20000

typedef u_char	le_buf_t;
#endif

/*
 * This structure is overlayed on the network dual-port RAM.
 * Currently 32 * 1518 receive plus 8 * 1518 transmit buffers plus
 * buffer descriptor rings.
 */
struct lereg2 {
	/* init block */		/* CHIP address */
	u_short	ler2_mode;		/* +0x0000 */
	LEPAD(pad0)
	u_short	ler2_padr0;		/* +0x0002 */
	LEPAD(pad1)
	u_short	ler2_padr1;		/* +0x0004 */
	LEPAD(pad2)
	u_short	ler2_padr2;		/* +0x0006 */
	LEPAD(pad3)
	u_short	ler2_ladrf0;		/* +0x0008 */
	LEPAD(pad4)
	u_short	ler2_ladrf1;		/* +0x000A */
	LEPAD(pad5)
	u_short	ler2_ladrf2;		/* +0x000C */
	LEPAD(pad6)
	u_short	ler2_ladrf3;		/* +0x000E */
	LEPAD(pad7)
	u_short	ler2_rdra;		/* +0x0010 */
	LEPAD(pad8)
	u_short	ler2_rlen;		/* +0x0012 */
	LEPAD(pad9)
	u_short	ler2_tdra;		/* +0x0014 */
	LEPAD(pad10)
	u_short	ler2_tlen;		/* +0x0016 */
	LEPAD(pad11)
	/* receive message descriptors */
	struct	lermd {			/* +0x0018 */
		u_short	rmd0;
		LEPAD(pad0)
		u_short	rmd1;
		LEPAD(pad1)
		short	rmd2;
		LEPAD(pad2)
		u_short	rmd3;
		LEPAD(pad3)
	} ler2_rmd[LERBUF];
	/* transmit message descriptors */
	struct	letmd {			/* +0x0058 */
		u_short	tmd0;
		LEPAD(pad0)
		u_short	tmd1;
		LEPAD(pad1)
		short	tmd2;
		LEPAD(pad2)
		u_short	tmd3;
		LEPAD(pad3)
	} ler2_tmd[LETBUF];
	le_buf_t	ler2_rbuf[LERBUF][LEMTU]; /* +0x0060 */
	le_buf_t	ler2_tbuf[LETBUF][LEMTU]; /* +0x2FD0 */
};

/*
 * Control and status bits -- lereg0
 */
#define	LE_IE		0x80		/* interrupt enable */
#define	LE_IR		0x40		/* interrupt requested */
#define	LE_LOCK		0x08		/* lock status register */
#define	LE_ACK		0x04		/* ack of lock */
#define	LE_JAB		0x02		/* loss of tx clock (???) */
#define LE_IPL(x)	((((x) >> 4) & 0x3) + 3)

/*
 * Control and status bits -- lereg1
 */
#define	LE_CSR0		0
#define	LE_CSR1		1
#define	LE_CSR2		2
#define	LE_CSR3		3

#define	LE_SERR		0x8000
#define	LE_BABL		0x4000
#define	LE_CERR		0x2000
#define	LE_MISS		0x1000
#define	LE_MERR		0x0800
#define	LE_RINT		0x0400
#define	LE_TINT		0x0200
#define	LE_IDON		0x0100
#define	LE_INTR		0x0080
#define	LE_INEA		0x0040
#define	LE_RXON		0x0020
#define	LE_TXON		0x0010
#define	LE_TDMD		0x0008
#define	LE_STOP		0x0004
#define	LE_STRT		0x0002
#define	LE_INIT		0x0001

#define	LE_BSWP		0x4
#define	LE_MODE		0x0

/*
 * Control and status bits -- lereg2
 */
#define	LE_OWN		0x8000
#define	LE_ERR		0x4000
#define	LE_STP		0x0200
#define	LE_ENP		0x0100

#define	LE_FRAM		0x2000
#define	LE_OFLO		0x1000
#define	LE_CRC		0x0800
#define	LE_RBUFF	0x0400

#define	LE_MORE		0x1000
#define	LE_ONE		0x0800
#define	LE_DEF		0x0400

#define	LE_TBUFF	0x8000
#define	LE_UFLO		0x4000
#define	LE_LCOL		0x1000
#define	LE_LCAR		0x0800
#define	LE_RTRY		0x0400
