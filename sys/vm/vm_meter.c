/*
 * Copyright (c) 1982, 1986, 1989 Regents of the University of California.
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
 *
 *	@(#)vm_meter.c	7.17 (Berkeley) 7/8/92
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <vm/vm.h>

struct	loadavg averunnable;		/* load average, of runnable procs */

int	maxslp = MAXSLP;
int	saferss = SAFERSS;

void
vmmeter()
{
	register unsigned *cp, *rp, *sp;

	if (time.tv_sec % 5 == 0)
		loadav(&averunnable);
	if (proc0.p_slptime > maxslp/2)
		wakeup((caddr_t)&proc0);
}

/*
 * Constants for averages over 1, 5, and 15 minutes
 * when sampling at 5 second intervals.
 */
fixpt_t	cexp[3] = {
	0.9200444146293232 * FSCALE,	/* exp(-1/12) */
	0.9834714538216174 * FSCALE,	/* exp(-1/60) */
	0.9944598480048967 * FSCALE,	/* exp(-1/180) */
};

/*
 * Compute a tenex style load average of a quantity on
 * 1, 5 and 15 minute intervals.
 */
void
loadav(avg)
	register struct loadavg *avg;
{
	register int i, nrun;
	register struct proc *p;

	for (nrun = 0, p = (struct proc *)allproc; p != NULL; p = p->p_nxt) {
		switch (p->p_stat) {
		case SSLEEP:
			if (p->p_pri > PZERO || p->p_slptime != 0)
				continue;
			/* fall through */
		case SRUN:
		case SIDL:
			nrun++;
		}
	}
	for (i = 0; i < 3; i++)
		avg->ldavg[i] = (cexp[i] * avg->ldavg[i] +
			nrun * FSCALE * (FSCALE - cexp[i])) >> FSHIFT;
#if defined(COMPAT_43) && (defined(vax) || defined(tahoe))
	for (i = 0; i < 3; i++)
		avenrun[i] = (double) avg->ldavg[i] / FSCALE;
#endif /* COMPAT_43 */
}

/*
 * Load average information
 */
/* ARGSUSED */
int
kinfo_loadavg(op, where, acopysize, arg, aneeded)
	int op;
	register char *where;
	int *acopysize, arg, *aneeded;
{
	int buflen, error;

	*aneeded = sizeof(averunnable);
	if (where == NULL)
		return (0);
	/*
	 * Check for enough buffering.
	 */
	buflen = *acopysize;
	if (buflen < sizeof(averunnable)) {
		*acopysize = 0;
		return (0);
	}
	/*
	 * Copyout averunnable structure.
	 */
	averunnable.fscale = FSCALE;
	if (error = copyout((caddr_t)&averunnable, where, sizeof(averunnable)))
		return (error);
	*acopysize = sizeof(averunnable);
	return (0);
}

/*
 * Calculate and return vmtotals structure.
 */
int
kinfo_meter(op, where, acopysize, arg, aneeded)
	int op;
	caddr_t where;
	int *acopysize, arg, *aneeded;
{
	struct vmtotal vmtotals;
	int error;

	*aneeded = sizeof(struct vmtotal);
	if (where == NULL)
		return (0);
	if (*acopysize < sizeof(struct vmtotal))
		return (EINVAL);
	vmtotal(&vmtotals);
	if (error = copyout((caddr_t)&vmtotals, where, sizeof(struct vmtotal)))
		return (error);
	*acopysize = sizeof(struct vmtotal);
	return (0);
}

/*
 * Calculate the current state of the system.
 * Done on demand from getkerninfo().
 */
void
vmtotal(totalp)
	register struct vmtotal *totalp;
{
	register struct proc *p;
	register vm_map_entry_t	entry;
	register vm_object_t object;
	register vm_map_t map;
	int paging;

	bzero(totalp, sizeof *totalp);
	/*
	 * Mark all objects as inactive.
	 */
	simple_lock(&vm_object_list_lock);
	object = (vm_object_t) queue_first(&vm_object_list);
	while (!queue_end(&vm_object_list, (queue_entry_t) object)) {
		object->flags &= ~OBJ_ACTIVE;
		object = (vm_object_t) queue_next(&object->object_list);
	}
	simple_unlock(&vm_object_list_lock);
	/*
	 * Calculate process statistics.
	 */
	for (p = (struct proc *)allproc; p != NULL; p = p->p_nxt) {
		if (p->p_flag & SSYS)
			continue;
		switch (p->p_stat) {
		case 0:
			continue;

		case SSLEEP:
		case SSTOP:
			if (p->p_flag & SLOAD) {
				if (p->p_pri <= PZERO)
					totalp->t_dw++;
				else if (p->p_slptime < maxslp)
					totalp->t_sl++;
			} else if (p->p_slptime < maxslp)
				totalp->t_sw++;
			if (p->p_slptime >= maxslp)
				continue;
			break;

		case SRUN:
		case SIDL:
			if (p->p_flag & SLOAD)
				totalp->t_rq++;
			else
				totalp->t_sw++;
			if (p->p_stat == SIDL)
				continue;
			break;
		}
		/*
		 * Note active objects.
		 */
		paging = 0;
		for (map = &p->p_vmspace->vm_map, entry = map->header.next;
		     entry != &map->header; entry = entry->next) {
			if (entry->is_a_map || entry->is_sub_map ||
			    entry->object.vm_object == NULL)
				continue;
			entry->object.vm_object->flags |= OBJ_ACTIVE;
			paging |= entry->object.vm_object->paging_in_progress;
		}
		if (paging)
			totalp->t_pw++;
	}
	/*
	 * Calculate object memory usage statistics.
	 */
	simple_lock(&vm_object_list_lock);
	object = (vm_object_t) queue_first(&vm_object_list);
	while (!queue_end(&vm_object_list, (queue_entry_t) object)) {
		totalp->t_vm += num_pages(object->size);
		totalp->t_rm += object->resident_page_count;
		if (object->flags & OBJ_ACTIVE) {
			totalp->t_avm += num_pages(object->size);
			totalp->t_arm += object->resident_page_count;
		}
		if (object->ref_count > 1) {
			/* shared object */
			totalp->t_vmshr += num_pages(object->size);
			totalp->t_rmshr += object->resident_page_count;
			if (object->flags & OBJ_ACTIVE) {
				totalp->t_avmshr += num_pages(object->size);
				totalp->t_armshr += object->resident_page_count;
			}
		}
		object = (vm_object_t) queue_next(&object->object_list);
	}
	totalp->t_free = cnt.v_free_count;
}
