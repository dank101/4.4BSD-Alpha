/*
 * Copyright (c) 1992 The Regents of the University of California
 * Copyright (c) 1990, 1992 Jan-Simon Pendry
 * All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * Jan-Simon Pendry.
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
 *	@(#)fdesc_vnops.c	7.2 (Berkeley) 7/13/92
 *
 * $Id: fdesc_vnops.c,v 1.7 1992/05/30 10:05:34 jsp Exp jsp $
 */

/*
 * /dev/fd Filesystem
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/filedesc.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/buf.h>
#include <sys/dirent.h>
#include <miscfs/fdesc/fdesc.h>

/*
 * vp is the current namei directory
 * ndp is the name to locate in that directory...
 */
fdesc_lookup(ap)
	struct vop_lookup_args /* {
		struct vnode * a_dvp;
		struct vnode ** a_vpp;
		struct componentname * a_cnp;
	} */ *ap;
{
	struct vnode **vpp = ap->a_vpp;
	struct vnode *dvp = ap->a_dvp;
	char *pname;
	struct proc *p;
	int nfiles;
	unsigned fd;
	int error;
	struct vnode *fvp;

#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_lookup(%x)\n", ap);
	printf("fdesc_lookup(dp = %x, vpp = %x, cnp = %x)\n", dvp, vpp, ap->a_cnp);
#endif
	pname = ap->a_cnp->cn_nameptr;
#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_lookup(%s)\n", pname);
#endif
	if (ap->a_cnp->cn_namelen == 1 && *pname == '.') {
		*vpp = dvp;
		VREF(dvp);	
		/*VOP_LOCK(dvp);*/
		return (0);
	}

	p = ap->a_cnp->cn_proc;
	nfiles = p->p_fd->fd_nfiles;

	fd = 0;
	while (*pname >= '0' && *pname <= '9') {
		fd = 10 * fd + *pname++ - '0';
		if (fd >= nfiles)
			break;
	}

#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_lookup: fd = %d, *pname = %x\n", fd, *pname);
#endif
	if (*pname != '\0') {
		error = ENOENT;
		goto bad;
	}

	if (fd >= nfiles || p->p_fd->fd_ofiles[fd] == NULL) {
		error = EBADF;
		goto bad;
	}

#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_lookup: allocate new vnode\n");
#endif
	error = getnewvnode(VT_UFS, dvp->v_mount, fdesc_vnodeop_p, &fvp);
	if (error)
		goto bad;
	MALLOC(fvp->v_data, void *, sizeof(struct fdescnode), M_TEMP, M_WAITOK);
	VTOFDESC(fvp)->f_fd = fd;
	*vpp = fvp;
#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_lookup: newvp = %x\n", fvp);
#endif
	return (0);

bad:;
	*vpp = NULL;
#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_lookup: error = %d\n", error);
#endif
	return (error);
}

fdesc_open(ap)
	struct vop_open_args /* {
		struct vnode *a_vp;
		int  a_mode;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;

	/*
	 * Can always open the root (modulo perms)
	 */
	if (vp->v_flag & VROOT)
		return (0);

	/*
	 * XXX Kludge: set ap->a_p->p_dupfd to contain the value of the
	 * the file descriptor being sought for duplication. The error 
	 * return ensures that the vnode for this device will be released
	 * by vn_open. Open will detect this special error and take the
	 * actions in dupfdopen.  Other callers of vn_open or VOP_OPEN
	 * will simply report the error.
	 */
	ap->a_p->p_dupfd = VTOFDESC(vp)->f_fd;	/* XXX */
	return (ENODEV);
}

static int
fdesc_attr(fd, vap, cred, p)
	int fd;
	struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
{
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	int error;

#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_attr: fd = %d, nfiles = %d\n", fd, fdp->fd_nfiles);
#endif
	if (fd >= fdp->fd_nfiles || (fp = fdp->fd_ofiles[fd]) == NULL) {
#ifdef FDESC_DIAGNOSTIC
		printf("fdesc_attr: fp = %x (EBADF)\n", fp);
#endif
		return (EBADF);
	}

	/*
	 * Can stat the underlying vnode, but not sockets because
	 * they don't use struct vattrs.  Well, we could convert from
	 * a struct stat back to a struct vattr, later...
	 */
	switch (fp->f_type) {
	case DTYPE_VNODE:
		error = VOP_GETATTR((struct vnode *) fp->f_data, vap, cred, p);
		break;

	case DTYPE_SOCKET:
		error = EOPNOTSUPP;
		break;

	default:
		panic("fdesc attr");
		break;
	}

#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_attr: returns error %d\n", error);
#endif
	return (error);
}

fdesc_getattr(ap)
	struct vop_getattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;
	struct vattr *vap = ap->a_vap;
	unsigned fd;
	int error;

	if (vp->v_flag & VROOT) {
#ifdef FDESC_DIAGNOSTIC
		printf("fdesc_getattr: stat rootdir\n");
#endif
		bzero((caddr_t) vap, sizeof(*vap));
		vattr_null(vap);
		vap->va_type = VDIR;
		vap->va_mode = S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
		vap->va_nlink = 2;
		vap->va_uid = 0;
		vap->va_gid = 0;
		vap->va_fsid = vp->v_mount->mnt_stat.f_fsid.val[0];
		vap->va_fileid = 2;
		/* vap->va_qsize = 0; */
		vap->va_size = DEV_BSIZE;
		vap->va_blocksize = DEV_BSIZE;
		microtime(&vap->va_atime);
		vap->va_mtime = vap->va_atime;
		vap->va_ctime = vap->va_ctime;
		vap->va_gen = 0;
		vap->va_flags = 0;
		vap->va_rdev = 0;
		/* vap->va_qbytes = 0; */
		vap->va_bytes = 0;
		return (0);
	}

	fd = VTOFDESC(vp)->f_fd;
	error = fdesc_attr(fd, vap, ap->a_cred, ap->a_p);
	if (error == 0)
		vp->v_type = vap->va_type;
	return (error);
}

fdesc_setattr(ap)
	struct vop_setattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	struct filedesc *fdp = ap->a_p->p_fd;
	struct file *fp;
	unsigned fd;
	int error;

	/*
	 * Can't mess with the root vnode
	 */
	if (ap->a_vp->v_flag & VROOT)
		return (EACCES);

	fd = VTOFDESC(ap->a_vp)->f_fd;
#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_setattr: fd = %d, nfiles = %d\n", fd, fdp->fd_nfiles);
#endif
	if (fd >= fdp->fd_nfiles || (fp = fdp->fd_ofiles[fd]) == NULL) {
#ifdef FDESC_DIAGNOSTIC
		printf("fdesc_setattr: fp = %x (EBADF)\n", fp);
#endif
		return (EBADF);
	}

	/*
	 * Can setattr the underlying vnode, but not sockets!
	 */
	switch (fp->f_type) {
	case DTYPE_VNODE:
		error = VOP_SETATTR((struct vnode *) fp->f_data, ap->a_vap, ap->a_cred, ap->a_p);
		break;

	case DTYPE_SOCKET:
		error = EOPNOTSUPP;
		break;

	default:
		panic("fdesc setattr");
		break;
	}

#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_setattr: returns error %d\n", error);
#endif
	return (error);
}

fdesc_readdir(ap)
	struct vop_readdir_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		struct ucred *a_cred;
	} */ *ap;
{
	struct uio *uio = ap->a_uio;
	struct filedesc *fdp;
	int i;
	int error;

#define UIO_MX 16

	fdp = uio->uio_procp->p_fd;
	i = uio->uio_offset / UIO_MX;
	error = 0;
	while (uio->uio_resid > 0) {
		if (i >= fdp->fd_nfiles) {
			/* *ap->a_eofflagp = 1; */
			break;
		}
		if (fdp->fd_ofiles[i] != NULL) {
			struct dirent d;
			struct dirent *dp = &d;
			char *cp = dp->d_name;
#ifdef FDESC_FILEID
			struct vattr va;
#endif
			bzero((caddr_t) dp, UIO_MX);

			dp->d_namlen = sprintf(dp->d_name, "%d", i);
			/*
			 * Fill in the remaining fields
			 */
			dp->d_reclen = UIO_MX;
			dp->d_type = DT_UNKNOWN;
			dp->d_fileno = i + 3;
#ifdef FDESC_FILEID
			/*
			 * If we want the file ids to match the
			 * we must call getattr on the underlying file.
			 * fdesc_attr may return an error, in which case
			 * we ignore the returned file id.
			 */
			error = fdesc_attr(i, &va, ap->a_cred, p);
			if (error == 0)
				dp->d_ino = va.va_fileid;
#endif
			/*
			 * And ship to userland
			 */
			error = uiomove((caddr_t) dp, UIO_MX, uio);
			if (error)
				break;
		}
		i++;
	}

	uio->uio_offset = i * UIO_MX;
	return (error);
}

fdesc_inactive(ap)
	struct vop_inactive_args /* {
		struct vnode *a_vp;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;

	/*
	 * Clear out the v_type field to avoid
	 * nasty things happening in vgone().
	 */
	vp->v_type = VNON;
#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_inactive(%x)\n", vp);
#endif
	return (0);
}

fdesc_reclaim(ap)
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;
	printf("fdesc_reclaim(%x)\n", vp);
	if (vp->v_data) {
		FREE(vp->v_data, M_TEMP);
		vp->v_data = 0;
	}
	return (0);
}

/*
 * Print out the contents of a /dev/fd vnode.
 */
/* ARGSUSED */
fdesc_print(ap)
	struct vop_print_args /* {
		struct vnode *a_vp;
	} */ *ap;
{

	printf("tag VT_NON, fdesc vnode\n");
	return (0);
}

/*void*/
fdesc_vfree(ap)
	struct vop_vfree_args /* {
		struct vnode *a_pvp;
		ino_t a_ino;
		int a_mode;
	} */ *ap;
{

	return (0);
}

/*
 * /dev/fd vnode unsupported operation
 */
fdesc_enotsupp()
{

	return (EOPNOTSUPP);
}

/*
 * /dev/fd "should never get here" operation
 */
fdesc_badop()
{

	panic("fdesc: bad op");
	/* NOTREACHED */
}

/*
 * /dev/fd vnode null operation
 */
fdesc_nullop()
{

	return (0);
}

#define fdesc_create ((int (*) __P((struct  vop_create_args *)))fdesc_enotsupp)
#define fdesc_mknod ((int (*) __P((struct  vop_mknod_args *)))fdesc_enotsupp)
#define fdesc_close ((int (*) __P((struct  vop_close_args *)))nullop)
#define fdesc_access ((int (*) __P((struct  vop_access_args *)))nullop)
#define fdesc_read ((int (*) __P((struct  vop_read_args *)))fdesc_enotsupp)
#define fdesc_write ((int (*) __P((struct  vop_write_args *)))fdesc_enotsupp)
#define fdesc_ioctl ((int (*) __P((struct  vop_ioctl_args *)))fdesc_enotsupp)
#define fdesc_select ((int (*) __P((struct  vop_select_args *)))fdesc_enotsupp)
#define fdesc_mmap ((int (*) __P((struct  vop_mmap_args *)))fdesc_enotsupp)
#define fdesc_fsync ((int (*) __P((struct  vop_fsync_args *)))nullop)
#define fdesc_seek ((int (*) __P((struct  vop_seek_args *)))nullop)
#define fdesc_remove ((int (*) __P((struct  vop_remove_args *)))fdesc_enotsupp)
#define fdesc_link ((int (*) __P((struct  vop_link_args *)))fdesc_enotsupp)
#define fdesc_rename ((int (*) __P((struct  vop_rename_args *)))fdesc_enotsupp)
#define fdesc_mkdir ((int (*) __P((struct  vop_mkdir_args *)))fdesc_enotsupp)
#define fdesc_rmdir ((int (*) __P((struct  vop_rmdir_args *)))fdesc_enotsupp)
#define fdesc_symlink ((int (*) __P((struct vop_symlink_args *)))fdesc_enotsupp)
#define fdesc_readlink \
	((int (*) __P((struct  vop_readlink_args *)))fdesc_enotsupp)
#define fdesc_abortop ((int (*) __P((struct  vop_abortop_args *)))nullop)
#define fdesc_lock ((int (*) __P((struct  vop_lock_args *)))nullop)
#define fdesc_unlock ((int (*) __P((struct  vop_unlock_args *)))nullop)
#define fdesc_bmap ((int (*) __P((struct  vop_bmap_args *)))fdesc_badop)
#define fdesc_strategy ((int (*) __P((struct  vop_strategy_args *)))fdesc_badop)
#define fdesc_islocked ((int (*) __P((struct  vop_islocked_args *)))nullop)
#define fdesc_advlock ((int (*) __P((struct vop_advlock_args *)))fdesc_enotsupp)
#define fdesc_blkatoff \
	((int (*) __P((struct  vop_blkatoff_args *)))fdesc_enotsupp)
#define fdesc_vget ((int (*) __P((struct  vop_vget_args *)))fdesc_enotsupp)
#define fdesc_valloc ((int(*) __P(( \
		struct vnode *pvp, \
		int mode, \
		struct ucred *cred, \
		struct vnode **vpp))) fdesc_enotsupp)
#define fdesc_truncate \
	((int (*) __P((struct  vop_truncate_args *)))fdesc_enotsupp)
#define fdesc_update ((int (*) __P((struct  vop_update_args *)))fdesc_enotsupp)
#define fdesc_bwrite ((int (*) __P((struct  vop_bwrite_args *)))fdesc_enotsupp)

int (**fdesc_vnodeop_p)();
struct vnodeopv_entry_desc fdesc_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, fdesc_lookup },	/* lookup */
	{ &vop_create_desc, fdesc_create },	/* create */
	{ &vop_mknod_desc, fdesc_mknod },	/* mknod */
	{ &vop_open_desc, fdesc_open },		/* open */
	{ &vop_close_desc, fdesc_close },	/* close */
	{ &vop_access_desc, fdesc_access },	/* access */
	{ &vop_getattr_desc, fdesc_getattr },	/* getattr */
	{ &vop_setattr_desc, fdesc_setattr },	/* setattr */
	{ &vop_read_desc, fdesc_read },		/* read */
	{ &vop_write_desc, fdesc_write },	/* write */
	{ &vop_ioctl_desc, fdesc_ioctl },	/* ioctl */
	{ &vop_select_desc, fdesc_select },	/* select */
	{ &vop_mmap_desc, fdesc_mmap },		/* mmap */
	{ &vop_fsync_desc, fdesc_fsync },	/* fsync */
	{ &vop_seek_desc, fdesc_seek },		/* seek */
	{ &vop_remove_desc, fdesc_remove },	/* remove */
	{ &vop_link_desc, fdesc_link },		/* link */
	{ &vop_rename_desc, fdesc_rename },	/* rename */
	{ &vop_mkdir_desc, fdesc_mkdir },	/* mkdir */
	{ &vop_rmdir_desc, fdesc_rmdir },	/* rmdir */
	{ &vop_symlink_desc, fdesc_symlink },	/* symlink */
	{ &vop_readdir_desc, fdesc_readdir },	/* readdir */
	{ &vop_readlink_desc, fdesc_readlink },	/* readlink */
	{ &vop_abortop_desc, fdesc_abortop },	/* abortop */
	{ &vop_inactive_desc, fdesc_inactive },	/* inactive */
	{ &vop_reclaim_desc, fdesc_reclaim },	/* reclaim */
	{ &vop_lock_desc, fdesc_lock },		/* lock */
	{ &vop_unlock_desc, fdesc_unlock },	/* unlock */
	{ &vop_bmap_desc, fdesc_bmap },		/* bmap */
	{ &vop_strategy_desc, fdesc_strategy },	/* strategy */
	{ &vop_print_desc, fdesc_print },	/* print */
	{ &vop_islocked_desc, fdesc_islocked },	/* islocked */
	{ &vop_advlock_desc, fdesc_advlock },	/* advlock */
	{ &vop_blkatoff_desc, fdesc_blkatoff },	/* blkatoff */
	{ &vop_valloc_desc, fdesc_valloc },	/* valloc */
	{ &vop_vfree_desc, fdesc_vfree },	/* vfree */
	{ &vop_truncate_desc, fdesc_truncate },	/* truncate */
	{ &vop_update_desc, fdesc_update },	/* update */
	{ &vop_bwrite_desc, fdesc_bwrite },	/* bwrite */
	{ (struct vnodeop_desc*)NULL, (int(*)())NULL }
};
struct vnodeopv_desc fdesc_vnodeop_opv_desc =
	{ &fdesc_vnodeop_p, fdesc_vnodeop_entries };
