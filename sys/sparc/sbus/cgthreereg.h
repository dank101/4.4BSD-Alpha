/*
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratories.
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
 *	@(#)cgthreereg.h	7.2 (Berkeley) 7/21/92
 *
 * from: $Header: cgthreereg.h,v 1.4 92/06/17 06:59:32 torek Exp $
 */

/*
 * cgthree display registers.
 *
 * The registers start at offset 0x400000 and repeat every 32 bytes
 * (presumably only the low order address lines are decoded).  Video RAM
 * starts at offset 0x800000.  We use separate pointers to each so that
 * the sparc addressing modes work well.
 *
 * The cg3 has a Brooktree Bt458 (?) chip to do everything (Brooktree
 * makes the only decent color frame buffer chips).  To update the
 * color map one would normally do byte writes, but the hardware
 * takes longword writes.  Since there are three registers for each
 * color map entry (R, then G, then B), we have to set color 1 with
 * a write to address 0 (setting 0's R/G/B and color 1's R) followed
 * by a second write to address 1 (setting color 1's G/B and color 2's
 * R/G).  Software must therefore keep a copy of the current map.
 *
 * The colormap address register increments automatically, so the above
 * write is done as:
 *
 *	p->cg3_cadr = 0;
 *	p->cg3_cmap = R0G0B0R1;
 *	p->cg3_cmap = G1B1R2G2;
 *
 * Yow!
 */
struct cgthreereg {
	u_int	cg3_addr;		/* ?any? address register */
	u_int	cg3_cmap;		/* colormap data register */
	u_int	cg3_ctrl;		/* control register */
	u_int	cg3_omap;		/* overlay map register */
	char	cg3_xxx0[16];		/* ? (make same size as bwtwo) */
};

/* offsets */
#define	CG3REG_ID	0
#define	CG3REG_REG	0x400000
#define	CG3REG_MEM	0x800000

/* same, but for gdb */
struct cgthree_all {
	long	ba_id;			/* ID = 0xfe010104 on my IPC */
	char	ba_xxx0[0x400000-4];
	struct	cgthreereg ba_reg;	/* control registers */
	char	ba_xxx1[0x400000-32];
	char	ba_ram[4096];		/* actually larger */
};
