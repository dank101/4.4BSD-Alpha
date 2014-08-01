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
 *	@(#)sun_misc.c	7.3 (Berkeley) 7/21/92
 *
 * from: $Header: sun_misc.c,v 1.12 92/07/12 13:26:10 torek Exp $
 */

/*
 * SunOS compatibility module.
 *
 * SunOS system calls that are implemented differently in BSD are
 * handled here.
 */

#include "param.h"
#include "proc.h"
#include "file.h"
#include "filedesc.h"
#include "ioctl.h"
#include "malloc.h"
#include "mbuf.h"
#include "mman.h"
#include "mount.h"
#include "resource.h"
#include "resourcevar.h"
#include "signal.h"
#include "signalvar.h"
#include "socket.h"
#include "vnode.h"
#include "uio.h"
#include "wait.h"

#include "miscfs/specfs/specdev.h"

#include "vm/vm.h"

struct sun_wait4_args {
	int	pid;
	int	*status;
	int	options;
	struct	rusage *rusage;
};
sun_wait4(p, uap, retval)
	struct proc *p;
	struct sun_wait4_args *uap;
	int *retval;
{

	if (uap->pid == 0)
		uap->pid = WAIT_ANY;
	return (wait4(p, uap, retval));
}

struct sun_creat_args {
	char	*fname;
	int	fmode;
};
sun_creat(p, uap, retval)
	struct proc *p;
	struct sun_creat_args *uap;
	int *retval;
{
	struct args {
		char	*fname;
		int	mode;
		int	crtmode;
	} openuap;

	openuap.fname = uap->fname;
	openuap.crtmode = uap->fmode;
	openuap.mode = O_WRONLY | O_CREAT | O_TRUNC;
	return (open(p, &openuap, retval));
}

struct sun_execv_args {
	char	*fname;
	char	**argp;
	char	**envp;		/* pseudo */
};
sun_execv(p, uap, retval)
	struct proc *p;
	struct sun_execv_args *uap;
	int *retval;
{

	uap->envp = NULL;
	return (execve(p, uap, retval));
}

struct sun_omsync_args {
	caddr_t	addr;
	int	len;
	int	flags;
};
sun_omsync(p, uap, retval)
	struct proc *p;
	struct sun_omsync_args *uap;
	int *retval;
{

	if (uap->flags)
		return (EINVAL);
	return (msync(p, uap, retval));
}

struct sun_unmount_args {
	char	*name;
	int	flags;	/* pseudo */
};
sun_unmount(p, uap, retval)
	struct proc *p;
	struct sun_unmount_args *uap;
	int *retval;
{

	uap->flags = MNT_NOFORCE;
	return (unmount(p, uap, retval));
}

static int
gettype(tptr)
	int *tptr;
{
	int type, error;
	char in[20];

	if (error = copyinstr((caddr_t)*tptr, in, sizeof in, (u_int *)0))
		return (error);
	if (strcmp(in, "4.2") == 0 || strcmp(in, "ufs") == 0)
		type = MOUNT_UFS;
	else if (strcmp(in, "nfs") == 0)
		type = MOUNT_NFS;
	else
		return (EINVAL);
	*tptr = type;
	return (0);
}

#define	SUNM_RDONLY	0x01	/* mount fs read-only */
#define	SUNM_NOSUID	0x02	/* mount fs with setuid disallowed */
#define	SUNM_NEWTYPE	0x04	/* type is string (char *), not int */
#define	SUNM_GRPID	0x08	/* (bsd semantics; ignored) */
#define	SUNM_REMOUNT	0x10	/* update existing mount */
#define	SUNM_NOSUB	0x20	/* prevent submounts (rejected) */
#define	SUNM_MULTI	0x40	/* (ignored) */
#define	SUNM_SYS5	0x80	/* Sys 5-specific semantics (rejected) */

struct sun_mount_args {
	int	type;
	char	*dir;
	int	flags;
	caddr_t	data;
};
sun_mount(p, uap, retval)
	struct proc *p;
	struct sun_mount_args *uap;
	int *retval;
{
	int oflags = uap->flags, nflags, error;

	if (oflags & (SUNM_NOSUB | SUNM_SYS5))
		return (EINVAL);
	if (oflags & SUNM_NEWTYPE && (error = gettype(&uap->type)))
		return (error);
	nflags = 0;
	if (oflags & SUNM_RDONLY)
		nflags |= MNT_RDONLY;
	if (oflags & SUNM_NOSUID)
		nflags |= MNT_NOSUID;
	if (oflags & SUNM_REMOUNT)
		nflags |= MNT_UPDATE;
	uap->flags = nflags;
	return (mount(p, uap, retval));
}

struct sun_sigpending_args {
	int	*mask;
};
sun_sigpending(p, uap, retval)
	struct proc *p;
	struct sun_sigpending_args *uap;
	int *retval;
{
	int mask = p->p_sig & p->p_sigmask;

	return (copyout((caddr_t)&mask, (caddr_t)uap->mask, sizeof(int)));
}

#if 0
/* here is the sun layout (not used directly): */
struct sun_dirent {
	long	d_off;
	u_long	d_fileno;
	u_short	d_reclen;
	u_short	d_namlen;
	char	d_name[256];
};
#endif
/* and the BSD layout: */
struct bsd_dirent {
	u_long	d_fileno;
	u_short	d_reclen;
	u_short	d_namlen;
	char	d_name[256];
};

/*
 * Read Sun-style directory entries.  We suck them into kernel space so
 * that they can be massaged before being copied out to user code.  Like
 * SunOS, we squish out `empty' entries.
 *
 * This is quite ugly, but what do you expect from compatibility code?
 */
struct sun_getdents_args {
	int	fd;
	char	*buf;
	int	nbytes;
};
sun_getdents(p, uap, retval)
	struct proc *p;
	register struct sun_getdents_args *uap;
	int *retval;
{
	register struct vnode *vp;
	register caddr_t inp, buf;	/* BSD-format */
	register int len, reclen;	/* BSD-format */
	register caddr_t outp;		/* Sun-format */
	register int resid;		/* Sun-format */
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	off_t off;			/* true file offset */
	long soff;			/* Sun file offset */
	int buflen, error, eofflag;
#define	SUN_RECLEN(reclen) (reclen + sizeof(long))

	if ((error = getvnode(p->p_fd, uap->fd, &fp)) != 0)
		return (error);
	if ((fp->f_flag & FREAD) == 0)
		return (EBADF);
	vp = (struct vnode *)fp->f_data;
	if (vp->v_type != VDIR)	/* XXX  vnode readdir op should do this */
		return (EINVAL);
	buflen = min(MAXBSIZE, uap->nbytes);
	buf = malloc(buflen, M_TEMP, M_WAITOK);
	VOP_LOCK(vp);
	off = fp->f_offset;
again:
	aiov.iov_base = buf;
	aiov.iov_len = buflen;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_READ;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_procp = p;
	auio.uio_resid = buflen;
	auio.uio_offset = off;
	/*
	 * First we read into the malloc'ed buffer, then
	 * we massage it into user space, one record at a time.
	 */
	if (error = VOP_READDIR(vp, &auio, fp->f_cred, &eofflag))
		goto out;
	inp = buf;
	outp = uap->buf;
	resid = uap->nbytes;
	if ((len = buflen - auio.uio_resid) == 0)
		goto eof;
	for (; len > 0; len -= reclen) {
		reclen = ((struct bsd_dirent *)inp)->d_reclen;
		if (reclen & 3)
			panic("sun_getdents");
		off += reclen;		/* each entry points to next */
		if (((struct bsd_dirent *)inp)->d_fileno == 0) {
			inp += reclen;	/* it is a hole; squish it out */
			continue;
		}
		if (reclen > len || resid < SUN_RECLEN(reclen)) {
			/* entry too big for buffer, so just stop */
			outp++;
			break;
		}
		/* copy out a Sun-shaped dirent */
		((struct bsd_dirent *)inp)->d_reclen = SUN_RECLEN(reclen);
		soff = off;
		if ((error = copyout((caddr_t)&soff, outp, sizeof soff)) != 0 ||
		    (error = copyout(inp, outp + sizeof soff, reclen)) != 0)
			goto out;
		/* advance past this real entry */
		inp += reclen;
		/* advance output past Sun-shaped entry */
		outp += SUN_RECLEN(reclen);
		resid -= SUN_RECLEN(reclen);
	}
	/* if we squished out the whole block, try again */
	if (outp == uap->buf)
		goto again;
	fp->f_offset = off;		/* update the vnode offset */
eof:
	*retval = uap->nbytes - resid;
out:
	VOP_UNLOCK(vp);
	free(buf, M_TEMP);
	return (error);
}

#define	MAXDOMAINNAME	64
char	sun_domainname[MAXDOMAINNAME];
int	sun_domainnamelen = 1;

struct sun_getdomainname_args {
	char	*name;
	int	namelen;
};
sun_getdomainname(p, uap, retval)
	struct proc *p;
	struct sun_getdomainname_args *uap;
	int *retval;
{
	register int l = min(uap->namelen, sun_domainnamelen + 1);

	return (copyout(sun_domainname, uap->name, l));
}

struct sun_setdomainname_args {
	char	*name;
	int	namelen;
};
sun_setdomainname(p, uap, retval)
	struct proc *p;
	struct sun_setdomainname_args *uap;
	int *retval;
{
	register int l = uap->namelen, error;

	if (l >= MAXDOMAINNAME)
		return (EINVAL);	/* ??? ENAMETOOLONG? */
	if (error = suser(p->p_ucred, &p->p_acflag))
		return (error);
	if (error = copyin(uap->name, sun_domainname, l))
		return (error);
	sun_domainname[l] = 0;
	return (0);
}

#define	SUN_MMAP_MASK	0xf		/* mask for SHARED/PRIVATE */
#define	SUN_MMAP_CANDO	0x80000000	/* if not, old mmap & cannot handle */

#define	DEVZERO	makedev(3, 12)		/* major,minor of /dev/zero */

#define	SUN_MMAP_SAME	(MAP_SHARED|MAP_PRIVATE|MAP_FIXED|MAP_INHERIT)

struct sun_mmap_args {
	caddr_t	addr;
	size_t	len;
	int	prot;
	int	flags;
	int	fd;
	long	off;		/* not off_t! */
	quad_t	qoff;		/* created here and fed to smmap() */
};
sun_mmap(p, uap, retval)
	register struct proc *p;
	register struct sun_mmap_args *uap;
	int *retval;
{
	register int flags;
	register struct filedesc *fdp;
	register struct file *fp;
	register struct vnode *vp;

	/*
	 * Verify the arguments.
	 */
	flags = uap->flags;
	if ((flags & SUN_MMAP_CANDO) == 0)
		return (EINVAL);
	if ((flags & SUN_MMAP_MASK) != MAP_SHARED &&
	    (flags & SUN_MMAP_MASK) != MAP_PRIVATE)
		return (EINVAL);
	flags &= ~SUN_MMAP_CANDO;

	/*
	 * Special case: if fd refers to /dev/zero, map as MAP_ANON.  (XXX)
	 */
	fdp = p->p_fd;
	if ((unsigned)uap->fd < fdp->fd_nfiles &&			/*XXX*/
	    (fp = fdp->fd_ofiles[uap->fd]) != NULL &&			/*XXX*/
	    fp->f_type == DTYPE_VNODE &&				/*XXX*/
	    (vp = (struct vnode *)fp->f_data)->v_type == VCHR &&	/*XXX*/
	    vp->v_rdev == DEVZERO) {					/*XXX*/
		flags |= MAP_ANON;
		uap->fd = -1;
	}

	/* All done, fix up fields and go. */
	uap->flags = flags;
	uap->qoff = (quad_t)uap->off;
	return (smmap(p, uap, retval));
}

#define	MC_SYNC		1
#define	MC_LOCK		2
#define	MC_UNLOCK	3
#define	MC_ADVISE	4
#define	MC_LOCKAS	5
#define	MC_UNLOCKAS	6

struct sun_mctl_args {
	caddr_t	addr;
	size_t	len;
	int	func;
	void	*arg;
};
sun_mctl(p, uap, retval)
	register struct proc *p;
	register struct sun_mctl_args *uap;
	int *retval;
{

	switch (uap->func) {

	case MC_ADVISE:		/* ignore for now */
		return (0);

	case MC_SYNC:		/* translate to msync */
		return (msync(p, uap, retval));

	default:
		return (EINVAL);
	}
}

struct sun_setreuid_args {
	int	ruid;		/* not uid_t */
	int	euid;
};
sun_setreuid(p, uap, retval)
	struct proc *p;
	struct sun_setreuid_args *uap;
	int *retval;
{
	register struct pcred *pc = p->p_cred;
	register uid_t ruid, euid;
	int error;

	if (uap->ruid == -1)
		ruid = pc->p_ruid;
	else
		ruid = uap->ruid;
	/*
	 * Allow setting real uid to previous effective, for swapping real and
	 * effective.  This should be:
	 *
	 * if (ruid != pc->p_ruid &&
	 *     (error = suser(pc->pc_ucred, &p->p_acflag)))
	 */
	if (ruid != pc->p_ruid && ruid != pc->pc_ucred->cr_uid /* XXX */ &&
	    (error = suser(pc->pc_ucred, &p->p_acflag)))
		return (error);
	if (uap->euid == -1)
		euid = pc->pc_ucred->cr_uid;
	else
		euid = uap->euid;
	if (euid != pc->pc_ucred->cr_uid && euid != pc->p_ruid &&
	    euid != pc->p_svuid && (error = suser(pc->pc_ucred, &p->p_acflag)))
		return (error);
	/*
	 * Everything's okay, do it.  Copy credentials so other references do
	 * not see our changes.
	 */
	pc->pc_ucred = crcopy(pc->pc_ucred);
	pc->pc_ucred->cr_uid = euid;
	pc->p_ruid = ruid;
	p->p_flag |= SUGID;
	return (0);
}

struct sun_setregid_args {
	int	rgid;		/* not gid_t */
	int	egid;
};
sun_setregid(p, uap, retval)
	struct proc *p;
	struct sun_setregid_args *uap;
	int *retval;
{
	register struct pcred *pc = p->p_cred;
	register gid_t rgid, egid;
	int error;

	if (uap->rgid == -1)
		rgid = pc->p_rgid;
	else
		rgid = uap->rgid;
	/*
	 * Allow setting real gid to previous effective, for swapping real and
	 * effective.  This didn't really work correctly in 4.[23], but is
	 * preserved so old stuff doesn't fail.  This should be:
	 *
	 * if (rgid != pc->p_rgid &&
	 *     (error = suser(pc->pc_ucred, &p->p_acflag)))
	 */
	if (rgid != pc->p_rgid && rgid != pc->pc_ucred->cr_groups[0] /* XXX */ &&
	    (error = suser(pc->pc_ucred, &p->p_acflag)))
		return (error);
	if (uap->egid == -1)
		egid = pc->pc_ucred->cr_groups[0];
	else
		egid = uap->egid;
	if (egid != pc->pc_ucred->cr_groups[0] && egid != pc->p_rgid &&
	    egid != pc->p_svgid && (error = suser(pc->pc_ucred, &p->p_acflag)))
		return (error);
	pc->pc_ucred = crcopy(pc->pc_ucred);
	pc->pc_ucred->cr_groups[0] = egid;
	pc->p_rgid = rgid;
	p->p_flag |= SUGID;
	return (0);
}

struct sun_setsockopt_args {
	int	s;
	int	level;
	int	name;
	caddr_t	val;
	int	valsize;
};
sun_setsockopt(p, uap, retval)
	struct proc *p;
	register struct sun_setsockopt_args *uap;
	int *retval;
{
	struct file *fp;
	struct mbuf *m = NULL;
	int error;

	if (error = getsock(p->p_fd, uap->s, &fp))
		return (error);
#define	SO_DONTLINGER (~SO_LINGER)
	if (uap->name == SO_DONTLINGER) {
		m = m_get(M_WAIT, MT_SOOPTS);
		if (m == NULL)
			return (ENOBUFS);
		mtod(m, struct linger *)->l_onoff = 0;
		m->m_len = sizeof(struct linger);
		return (sosetopt((struct socket *)fp->f_data, uap->level,
		    SO_LINGER, m));
	}
	if (uap->valsize > MLEN)
		return (EINVAL);
	if (uap->val) {
		m = m_get(M_WAIT, MT_SOOPTS);
		if (m == NULL)
			return (ENOBUFS);
		if (error = copyin(uap->val, mtod(m, caddr_t),
		    (u_int)uap->valsize)) {
			(void) m_free(m);
			return (error);
		}
		m->m_len = uap->valsize;
	}
	return (sosetopt((struct socket *)fp->f_data, uap->level,
	    uap->name, m));
}

struct sun_fchroot_args {
	int	fdes;
};
sun_fchroot(p, uap, retval)
	register struct proc *p;
	register struct sun_fchroot_args *uap;
	int *retval;
{
	register struct filedesc *fdp = p->p_fd;
	register struct vnode *vp;
	struct file *fp;
	int error;

	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return (error);
	if ((error = getvnode(fdp, uap->fdes, &fp)) != 0)
		return (error);
	vp = (struct vnode *)fp->f_data;
	VOP_LOCK(vp);
	if (vp->v_type != VDIR)
		error = ENOTDIR;
	else
		error = VOP_ACCESS(vp, VEXEC, p->p_ucred, p);
	VOP_UNLOCK(vp);
	if (error)
		return (error);
	VREF(vp);
	if (fdp->fd_rdir != NULL)
		vrele(fdp->fd_rdir);
	fdp->fd_rdir = vp;
	return (0);
}
