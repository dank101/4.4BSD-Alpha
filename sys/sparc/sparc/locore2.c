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
 *	@(#)locore2.c	7.2 (Berkeley) 7/21/92
 *
 * from: $Header: locore2.c,v 1.7 92/06/20 08:47:28 mccanne Exp $ (LBL)
 */

/*
 * Primitives which are in locore.s on other machines,
 * but which have no reason to be assembly-coded on SPARC.
 */

#include "param.h"
#include "proc.h"
#include "resourcevar.h"

int	whichqs;

/*
 * Put process p on the run queue indicated by its priority.
 * Calls should be made at splstatclock(), and p->p_stat should be SRUN.
 */
setrq(p)
	register struct proc *p;
{
	register struct prochd *q;
	register struct proc *oldlast;
	register int which = p->p_pri >> 2;

	if (p->p_rlink != NULL)
		panic("setrq");
	q = &qs[which];
	whichqs |= 1 << which;
	p->p_link = (struct proc *)q;
	p->p_rlink = oldlast = q->ph_rlink;
	q->ph_rlink = p;
	oldlast->p_link = p;
}

/*
 * Remove process p from its run queue, which should be the one
 * indicated by its priority.  Calls should be made at splstatclock().
 */
remrq(p)
	register struct proc *p;
{
	register int which = p->p_pri >> 2;
	register struct prochd *q;

	if ((whichqs & (1 << which)) == 0)
		panic("remrq");
	p->p_link->p_rlink = p->p_rlink;
	p->p_rlink->p_link = p->p_link;
	p->p_rlink = NULL;
	q = &qs[which];
	if (q->ph_link == (struct proc *)q)
		whichqs &= ~(1 << which);
}
