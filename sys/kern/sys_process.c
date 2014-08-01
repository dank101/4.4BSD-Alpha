/*-
 * Copyright (c) 1982, 1986, 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This module is believed to contain source code proprietary to AT&T.
 * Use and redistribution is subject to the Berkeley Software License
 * Agreement and your Software Agreement with AT&T (Western Electric).
 *
 *	@(#)sys_process.c	7.31 (Berkeley) 7/15/92
 */

#define IPCREG
#include "param.h"
#include "proc.h"
#include "vnode.h"
#include "buf.h"
#include "ptrace.h"

#include "machine/reg.h"
#include "machine/psl.h"
#include "vm/vm.h"
#include "vm/vm_page.h"

#include "user.h"

/*
 * Priority for tracing
 */
#define	IPCPRI	PZERO

/*
 * Tracing variables.
 * Used to pass trace command from
 * parent to child being traced.
 * This data base cannot be
 * shared and is locked
 * per user.
 */
struct {
	int	ip_lock;
	int	ip_req;
	int	*ip_addr;
	int	ip_data;
} ipc;

/*
 * Process debugging system call.
 */
struct ptrace_args {
	int	req;
	int	pid;
	int	*addr;
	int	data;
};
ptrace(curp, uap, retval)
	struct proc *curp;
	register struct ptrace_args *uap;
	int *retval;
{
	register struct proc *p;
	int error;

	p = pfind(uap->pid);
	if (uap->req == PT_ATTACH) {
		/*
		 * Must be root if the process has used set user or
		 * group privileges or does not belong to the real
		 * user. Must not be already traced.
		 */
		if ((p->p_flag & SUGID ||
		    p->p_cred->p_ruid != curp->p_cred->p_ruid) &&
		    (error = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (error);
		if (p->p_flag & STRC)
			return (EALREADY);	/* ??? */
		/*
		 * It would be nice if the tracing relationship was separate
		 * from the parent relationship but that would require
		 * another set of links in the proc struct or for "wait"
		 * to scan the entire proc table.  To make life easier,
		 * we just re-parent the process we're trying to trace.
		 * The old parent is remembered so we can put things back
		 * on a "detach".
		 */
		p->p_flag |= STRC;
		p->p_oppid = p->p_pptr->p_pid;
		proc_reparent(p, curp);
		psignal(p, SIGSTOP);
		return (0);
	}
	if (uap->req <= 0) {
		curp->p_flag |= STRC;
		return (0);
	}
	if (p == 0 || p->p_stat != SSTOP || p->p_pptr != curp ||
	    !(p->p_flag & STRC))
		return (ESRCH);
	while (ipc.ip_lock)
		sleep((caddr_t)&ipc, IPCPRI);
	ipc.ip_lock = p->p_pid;
	ipc.ip_data = uap->data;
	ipc.ip_addr = uap->addr;
	ipc.ip_req = uap->req;
	p->p_flag &= ~SWTED;
	while (ipc.ip_req > 0) {
		if (p->p_stat==SSTOP)
			setrun(p);
		sleep((caddr_t)&ipc, IPCPRI);
	}
	*retval = ipc.ip_data;
	ipc.ip_lock = 0;
	wakeup((caddr_t)&ipc);
	if (ipc.ip_req < 0)
		return (EIO);
	return (0);
}

#define	PHYSOFF(p, o) ((caddr_t)(p) + (o))

#if defined(i386)
#undef        PC
#undef        SP
#undef        PS
#undef        R0
#undef        R1

#define       PC      tEIP
#define       SP      tESP
#define       PS      tEFLAGS
#define       R0      tEDX
#define       R1      tECX
#endif

/*
 * Transmit a tracing request from the parent to the child process
 * being debugged. This code runs in the context of the child process
 * to fulfill the command requested by the parent.
 */
procxmt(p)
	register struct proc *p;
{
	register int i, *poff;
	extern char kstack[];

	if (ipc.ip_lock != p->p_pid)
		return (0);
	p->p_slptime = 0;
	p->p_addr->u_kproc.kp_proc.p_md.md_regs = p->p_md.md_regs; /* u.u_ar0 */
	i = ipc.ip_req;
	ipc.ip_req = 0;
	switch (i) {

	case PT_READ_I:			/* read the child's text space */
		if (!useracc((caddr_t)ipc.ip_addr, 4, B_READ))
			goto error;
		ipc.ip_data = fuiword((caddr_t)ipc.ip_addr);
		break;

	case PT_READ_D:			/* read the child's data space */
		if (!useracc((caddr_t)ipc.ip_addr, 4, B_READ))
			goto error;
		ipc.ip_data = fuword((caddr_t)ipc.ip_addr);
		break;

	case PT_READ_U:			/* read the child's u. */
#ifdef HPUXCOMPAT
		if (p->p_addr->u_pcb.pcb_flags & PCB_HPUXTRACE)
			i = hpuxtobsduoff(ipc.ip_addr);
		else
#endif
		i = (int)ipc.ip_addr;
		if ((u_int) i > ctob(UPAGES)-sizeof(int) || (i & 1) != 0)
			goto error;
		ipc.ip_data = *(int *)PHYSOFF(p->p_addr, i);
		break;

	case PT_WRITE_I:		/* write the child's text space */
		if ((i = suiword((caddr_t)ipc.ip_addr, ipc.ip_data)) < 0) {
			vm_offset_t sa, ea;
			int rv;

			sa = trunc_page((vm_offset_t)ipc.ip_addr);
			ea = round_page((vm_offset_t)ipc.ip_addr+sizeof(int));
			rv = vm_map_protect(&p->p_vmspace->vm_map, sa, ea,
					VM_PROT_DEFAULT, FALSE);
			if (rv == KERN_SUCCESS) {
				i = suiword((caddr_t)ipc.ip_addr, ipc.ip_data);
				(void) vm_map_protect(&p->p_vmspace->vm_map,
					sa, ea, VM_PROT_READ|VM_PROT_EXECUTE,
					FALSE);
			}
		}
		if (i < 0)
			goto error;
		break;

	case PT_WRITE_D:		/* write the child's data space */
		if (suword((caddr_t)ipc.ip_addr, 0) < 0)
			goto error;
		(void) suword((caddr_t)ipc.ip_addr, ipc.ip_data);
		break;

	case PT_WRITE_U:		/* write the child's u. */
#ifdef HPUXCOMPAT
		if (p->p_addr->u_pcb.pcb_flags & PCB_HPUXTRACE)
			i = hpuxtobsduoff(ipc.ip_addr);
		else
#endif
		i = (int)ipc.ip_addr;
#ifdef mips
		poff = (int *)PHYSOFF(curproc->p_addr, i);
#else
		poff = (int *)PHYSOFF(kstack, i);
#endif
		for (i=0; i<NIPCREG; i++)
			if (poff == &p->p_md.md_regs[ipcreg[i]])
				goto ok;
		if (poff == &p->p_md.md_regs[PS]) {
			ipc.ip_data |= PSL_USERSET;
			ipc.ip_data &= ~PSL_USERCLR;
#ifdef PSL_CM_CLR
			if (ipc.ip_data & PSL_CM)
				ipc.ip_data &= ~PSL_CM_CLR;
#endif
			goto ok;
		}
#if defined(hp300) || defined(luna68k)
#ifdef FPCOPROC
		if (poff >= (int *)&((struct user *)kstack)->u_pcb.pcb_fpregs.fpf_regs &&
		    poff <= (int *)&((struct user *)kstack)->u_pcb.pcb_fpregs.fpf_fpiar)
			goto ok;
#endif
#endif
		goto error;

	ok:
		*poff = ipc.ip_data;
		break;

	case PT_STEP:			/* single step the child */
	case PT_CONTINUE:		/* continue the child */
		if ((unsigned)ipc.ip_data >= NSIG)
			goto error;
		if ((int)ipc.ip_addr != 1)
			p->p_md.md_regs[PC] = (int)ipc.ip_addr;
		p->p_xstat = ipc.ip_data;	/* see issig */
#ifdef PSL_T
		/* need something more machine independent here... */
		if (i == PT_STEP) 
			p->p_md.md_regs[PS] |= PSL_T;
#endif
		wakeup((caddr_t)&ipc);
		return (1);

	case PT_KILL:			/* kill the child process */
		wakeup((caddr_t)&ipc);
		exit(p, (int)p->p_xstat);

	case PT_DETACH:			/* stop tracing the child */
		if ((unsigned)ipc.ip_data >= NSIG)
			goto error;
		if ((int)ipc.ip_addr != 1)
			p->p_md.md_regs[PC] = (int)ipc.ip_addr;
		p->p_xstat = ipc.ip_data;	/* see issig */
		p->p_flag &= ~STRC;
		if (p->p_oppid != p->p_pptr->p_pid) {
                        register struct proc *pp = pfind(p->p_oppid);

                        if (pp)
                                proc_reparent(p, pp);
		}
		p->p_oppid = 0;
		wakeup((caddr_t)&ipc);
		return (1);

	default:
	error:
		ipc.ip_req = -1;
	}
	wakeup((caddr_t)&ipc);
	return (0);
}
