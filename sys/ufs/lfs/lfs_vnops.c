/*
 * Copyright (c) 1986, 1989, 1991 Regents of the University of California.
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
 *	@(#)lfs_vnops.c	7.90 (Berkeley) 8/1/92
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/malloc.h>

#include <vm/vm.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/fifofs/fifo.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/ufs_extern.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

/* Global vfs data structures for lfs. */
int (**lfs_vnodeop_p)();
struct vnodeopv_entry_desc lfs_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, ufs_lookup },		/* lookup */
	{ &vop_create_desc, lfs_create },		/* create */
	{ &vop_mknod_desc, lfs_mknod },			/* mknod */
	{ &vop_open_desc, ufs_open },			/* open */
	{ &vop_close_desc, ufs_close },			/* close */
	{ &vop_access_desc, ufs_access },		/* access */
	{ &vop_getattr_desc, ufs_getattr },		/* getattr */
	{ &vop_setattr_desc, ufs_setattr },		/* setattr */
	{ &vop_read_desc, lfs_read },			/* read */
	{ &vop_write_desc, lfs_write },			/* write */
	{ &vop_ioctl_desc, ufs_ioctl },			/* ioctl */
	{ &vop_select_desc, ufs_select },		/* select */
	{ &vop_mmap_desc, ufs_mmap },			/* mmap */
	{ &vop_fsync_desc, lfs_fsync },			/* fsync */
	{ &vop_seek_desc, ufs_seek },			/* seek */
	{ &vop_remove_desc, lfs_remove },		/* remove */
	{ &vop_link_desc, lfs_link },			/* link */
	{ &vop_rename_desc, lfs_rename },		/* rename */
	{ &vop_mkdir_desc, lfs_mkdir },			/* mkdir */
	{ &vop_rmdir_desc, lfs_rmdir },			/* rmdir */
	{ &vop_symlink_desc, lfs_symlink },		/* symlink */
	{ &vop_readdir_desc, ufs_readdir },		/* readdir */
	{ &vop_readlink_desc, ufs_readlink },		/* readlink */
	{ &vop_abortop_desc, ufs_abortop },		/* abortop */
	{ &vop_inactive_desc, lfs_inactive },		/* inactive */
	{ &vop_reclaim_desc, ufs_reclaim },		/* reclaim */
	{ &vop_lock_desc, ufs_lock },			/* lock */
	{ &vop_unlock_desc, ufs_unlock },		/* unlock */
	{ &vop_bmap_desc, lfs_bmap },			/* bmap */
	{ &vop_strategy_desc, ufs_strategy },		/* strategy */
	{ &vop_print_desc, ufs_print },			/* print */
	{ &vop_islocked_desc, ufs_islocked },		/* islocked */
	{ &vop_advlock_desc, ufs_advlock },		/* advlock */
	{ &vop_blkatoff_desc, lfs_blkatoff },		/* blkatoff */
	{ &vop_valloc_desc, lfs_valloc },		/* valloc */
	{ &vop_vfree_desc, lfs_vfree },			/* vfree */
	{ &vop_truncate_desc, lfs_truncate },		/* truncate */
	{ &vop_update_desc, lfs_update },		/* update */
	{ &vop_bwrite_desc, lfs_bwrite },		/* bwrite */
	{ (struct vnodeop_desc*)NULL, (int(*)())NULL }
};
struct vnodeopv_desc lfs_vnodeop_opv_desc =
	{ &lfs_vnodeop_p, lfs_vnodeop_entries };

int (**lfs_specop_p)();
struct vnodeopv_entry_desc lfs_specop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, spec_lookup },		/* lookup */
	{ &vop_create_desc, spec_create },		/* create */
	{ &vop_mknod_desc, spec_mknod },		/* mknod */
	{ &vop_open_desc, spec_open },			/* open */
	{ &vop_close_desc, ufsspec_close },		/* close */
	{ &vop_access_desc, ufs_access },		/* access */
	{ &vop_getattr_desc, ufs_getattr },		/* getattr */
	{ &vop_setattr_desc, ufs_setattr },		/* setattr */
	{ &vop_read_desc, ufsspec_read },		/* read */
	{ &vop_write_desc, ufsspec_write },		/* write */
	{ &vop_ioctl_desc, spec_ioctl },		/* ioctl */
	{ &vop_select_desc, spec_select },		/* select */
	{ &vop_mmap_desc, spec_mmap },			/* mmap */
	{ &vop_fsync_desc, spec_fsync },		/* fsync */
	{ &vop_seek_desc, spec_seek },			/* seek */
	{ &vop_remove_desc, spec_remove },		/* remove */
	{ &vop_link_desc, spec_link },			/* link */
	{ &vop_rename_desc, spec_rename },		/* rename */
	{ &vop_mkdir_desc, spec_mkdir },		/* mkdir */
	{ &vop_rmdir_desc, spec_rmdir },		/* rmdir */
	{ &vop_symlink_desc, spec_symlink },		/* symlink */
	{ &vop_readdir_desc, spec_readdir },		/* readdir */
	{ &vop_readlink_desc, spec_readlink },		/* readlink */
	{ &vop_abortop_desc, spec_abortop },		/* abortop */
	{ &vop_inactive_desc, lfs_inactive },		/* inactive */
	{ &vop_reclaim_desc, ufs_reclaim },		/* reclaim */
	{ &vop_lock_desc, ufs_lock },			/* lock */
	{ &vop_unlock_desc, ufs_unlock },		/* unlock */
	{ &vop_bmap_desc, spec_bmap },			/* bmap */
	{ &vop_strategy_desc, spec_strategy },		/* strategy */
	{ &vop_print_desc, ufs_print },			/* print */
	{ &vop_islocked_desc, ufs_islocked },		/* islocked */
	{ &vop_advlock_desc, spec_advlock },		/* advlock */
	{ &vop_blkatoff_desc, spec_blkatoff },		/* blkatoff */
	{ &vop_valloc_desc, spec_valloc },		/* valloc */
	{ &vop_vfree_desc, lfs_vfree },			/* vfree */
	{ &vop_truncate_desc, spec_truncate },		/* truncate */
	{ &vop_update_desc, lfs_update },		/* update */
	{ &vop_bwrite_desc, lfs_bwrite },		/* bwrite */
	{ (struct vnodeop_desc*)NULL, (int(*)())NULL }
};
struct vnodeopv_desc lfs_specop_opv_desc =
	{ &lfs_specop_p, lfs_specop_entries };

#ifdef FIFO
int (**lfs_fifoop_p)();
struct vnodeopv_entry_desc lfs_fifoop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, fifo_lookup },		/* lookup */
	{ &vop_create_desc, fifo_create },		/* create */
	{ &vop_mknod_desc, fifo_mknod },		/* mknod */
	{ &vop_open_desc, fifo_open },			/* open */
	{ &vop_close_desc, ufsfifo_close },		/* close */
	{ &vop_access_desc, ufs_access },		/* access */
	{ &vop_getattr_desc, ufs_getattr },		/* getattr */
	{ &vop_setattr_desc, ufs_setattr },		/* setattr */
	{ &vop_read_desc, ufsfifo_read },		/* read */
	{ &vop_write_desc, ufsfifo_write },		/* write */
	{ &vop_ioctl_desc, fifo_ioctl },		/* ioctl */
	{ &vop_select_desc, fifo_select },		/* select */
	{ &vop_mmap_desc, fifo_mmap },			/* mmap */
	{ &vop_fsync_desc, fifo_fsync },		/* fsync */
	{ &vop_seek_desc, fifo_seek },			/* seek */
	{ &vop_remove_desc, fifo_remove },		/* remove */
	{ &vop_link_desc, fifo_link },			/* link */
	{ &vop_rename_desc, fifo_rename },		/* rename */
	{ &vop_mkdir_desc, fifo_mkdir },		/* mkdir */
	{ &vop_rmdir_desc, fifo_rmdir },		/* rmdir */
	{ &vop_symlink_desc, fifo_symlink },		/* symlink */
	{ &vop_readdir_desc, fifo_readdir },		/* readdir */
	{ &vop_readlink_desc, fifo_readlink },		/* readlink */
	{ &vop_abortop_desc, fifo_abortop },		/* abortop */
	{ &vop_inactive_desc, lfs_inactive },		/* inactive */
	{ &vop_reclaim_desc, ufs_reclaim },		/* reclaim */
	{ &vop_lock_desc, ufs_lock },			/* lock */
	{ &vop_unlock_desc, ufs_unlock },		/* unlock */
	{ &vop_bmap_desc, fifo_bmap },			/* bmap */
	{ &vop_strategy_desc, fifo_strategy },		/* strategy */
	{ &vop_print_desc, ufs_print },			/* print */
	{ &vop_islocked_desc, ufs_islocked },		/* islocked */
	{ &vop_advlock_desc, fifo_advlock },		/* advlock */
	{ &vop_blkatoff_desc, fifo_blkatoff },		/* blkatoff */
	{ &vop_valloc_desc, fifo_valloc },		/* valloc */
	{ &vop_vfree_desc, lfs_vfree },			/* vfree */
	{ &vop_truncate_desc, fifo_truncate },		/* truncate */
	{ &vop_update_desc, lfs_update },		/* update */
	{ &vop_bwrite_desc, lfs_bwrite },		/* bwrite */
	{ (struct vnodeop_desc*)NULL, (int(*)())NULL }
};
struct vnodeopv_desc lfs_fifoop_opv_desc =
	{ &lfs_fifoop_p, lfs_fifoop_entries };
#endif /* FIFO */

/*
 * Vnode op for reading.
 */
/* ARGSUSED */
lfs_read(ap)
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct inode *ip = VTOI(vp);
	register struct uio *uio = ap->a_uio;
	register struct lfs *fs;
	struct buf *bp;
	daddr_t lbn, bn, rablock;
	off_t diff;
	int error = 0, size;
	long n, on;

#ifdef DIAGNOSTIC
	int type;
	if (uio->uio_rw != UIO_READ)
		panic("lfs_read mode");
	type = ip->i_mode & IFMT;
	if (type != IFDIR && type != IFREG && type != IFLNK)
		panic("lfs_read type");
	if (type == IFLNK && (int)ip->i_size < vp->v_mount->mnt_maxsymlinklen)
		panic("read short symlink");
#endif
	if (uio->uio_resid == 0)
		return (0);
	fs = ip->i_lfs;
	if (uio->uio_offset < 0 ||
	    (u_quad_t)uio->uio_offset + uio->uio_resid > fs->lfs_maxfilesize)
		return (EFBIG);
	ip->i_flag |= IACC;
	do {
		lbn = lblkno(fs, uio->uio_offset);
		on = blkoff(fs, uio->uio_offset);
		n = min((unsigned)(fs->lfs_bsize - on), uio->uio_resid);
		diff = ip->i_size - uio->uio_offset;
		if (diff <= 0)
			return (0);
		if (diff < n)
			n = diff;
		size = blksize(fs);
		rablock = lbn + 1;
		if (vp->v_lastr + 1 == lbn &&
		    lblktosize(fs, rablock) < ip->i_size)
			error = breadn(ITOV(ip), lbn, size, &rablock,
				&size, 1, NOCRED, &bp);
		else
			error = bread(ITOV(ip), lbn, size, NOCRED, &bp);
		vp->v_lastr = lbn;
		n = min(n, size - bp->b_resid);
		if (error) {
			brelse(bp);
			return (error);
		}
		error = uiomove(bp->b_un.b_addr + on, (int)n, uio);
		if (n + on == fs->lfs_bsize || uio->uio_offset == ip->i_size)
			bp->b_flags |= B_AGE;
		brelse(bp);
	} while (error == 0 && uio->uio_resid > 0 && n != 0);
	return (error);
}

/*
 * Vnode op for writing.
 */
lfs_write(ap)
	struct vop_write_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct uio *uio = ap->a_uio;
	struct proc *p = uio->uio_procp;
	register struct inode *ip = VTOI(vp);
	register struct lfs *fs;
	register ioflag = ap->a_ioflag;
	struct timeval tv;
	struct buf *bp;
	daddr_t lbn;
	off_t osize;
	int n, on, flags, newblock;
	int size, resid, error = 0;

#ifdef DIAGNOSTIC
	if (uio->uio_rw != UIO_WRITE)
		panic("lfs_write mode");
#endif
	switch (vp->v_type) {
	case VREG:
		if (ioflag & IO_APPEND)
			uio->uio_offset = ip->i_size;
		/* fall through */
	case VLNK:
		break;

	case VDIR:
		/* XXX This may not be correct for LFS. */
		if ((ioflag & IO_SYNC) == 0)
			panic("lfs_write nonsync dir write");
		break;

	default:
		panic("lfs_write type");
	}
	if (uio->uio_offset < 0)
		return (EINVAL);
	if (uio->uio_resid == 0)
		return (0);
	/*
	 * Maybe this should be above the vnode op call, but so long as
	 * file servers have no limits, i don't think it matters
	 */
	if (vp->v_type == VREG && p &&
	    uio->uio_offset + uio->uio_resid >
	      p->p_rlimit[RLIMIT_FSIZE].rlim_cur) {
		psignal(p, SIGXFSZ);
		return (EFBIG);
	}
	resid = uio->uio_resid;
	osize = ip->i_size;
	fs = ip->i_lfs;						/* LFS */
	if (uio->uio_offset < 0 ||
	    (u_quad_t)uio->uio_offset + uio->uio_resid > fs->lfs_maxfilesize)
		return (EFBIG);
	flags = 0;
#ifdef NOTLFS
	if (ioflag & IO_SYNC)
		flags = B_SYNC;
#endif
	do {
		lbn = lblkno(fs, uio->uio_offset);
		on = blkoff(fs, uio->uio_offset);
		n = min((unsigned)(fs->lfs_bsize - on), uio->uio_resid);
		if (error = lfs_balloc(vp, n, lbn, &bp))
			break;
		if (uio->uio_offset + n > ip->i_size) {
			ip->i_size = uio->uio_offset + n;
			vnode_pager_setsize(vp, (u_long)ip->i_size);
		}
		size = blksize(fs);
		(void) vnode_pager_uncache(vp);
		n = min(n, size - bp->b_resid);
		error = uiomove(bp->b_un.b_addr + on, n, uio);
#ifdef NOTLFS							/* LFS */
		if (ioflag & IO_SYNC)
			(void) bwrite(bp);
		else if (n + on == fs->fs_bsize) {
			bp->b_flags |= B_AGE;
			bawrite(bp);
		} else
			bdwrite(bp);
		ip->i_flag |= IUPD|ICHG;
#else
		/* XXX This doesn't handle IO_SYNC. */
		LFS_UBWRITE(bp);
#endif
		if (ap->a_cred->cr_uid != 0)
			ip->i_mode &= ~(ISUID|ISGID);
	} while (error == 0 && uio->uio_resid > 0 && n != 0);
	if (error && (ioflag & IO_UNIT)) {
		(void)VOP_TRUNCATE(vp, osize, ioflag & IO_SYNC, ap->a_cred,
		    uio->uio_procp);
		uio->uio_offset -= resid - uio->uio_resid;
		uio->uio_resid = resid;
	}
	if (!error && (ioflag & IO_SYNC)) {
		tv = time;
		error = VOP_UPDATE(vp, &tv, &tv, 1);
	}
	return (error);
}

/*
 * Synch an open file.
 */
/* ARGSUSED */
lfs_fsync(ap)
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		struct ucred *a_cred;
		int a_waitfor;
		struct proc *a_p;
	} */ *ap;
{
	struct timeval tv;

	tv = time;
	return (VOP_UPDATE(ap->a_vp, &tv, &tv,
	    ap->a_waitfor == MNT_WAIT ? LFS_SYNC : 0));
}

/*
 * Last reference to an inode, write the inode out and if necessary,
 * truncate and deallocate the file.
 */
int
lfs_inactive(ap)
	struct vop_inactive_args /* {
		struct vnode *a_vp;
	} */ *ap;
{
	extern int prtactive;
	register struct vnode *vp = ap->a_vp;
	register struct inode *ip;
	struct timeval tv;
	int mode, error;

	if (prtactive && vp->v_usecount != 0)
		vprint("lfs_inactive: pushing active", vp);

	/* Get rid of inodes related to stale file handles. */
	ip = VTOI(vp);
	if (ip->i_mode == 0) {
		if ((vp->v_flag & VXLOCK) == 0)
			vgone(vp);
		return (0);
	}

	error = 0;
	ILOCK(ip);
	if (ip->i_nlink <= 0 && (vp->v_mount->mnt_flag & MNT_RDONLY) == 0) {
#ifdef QUOTA
		if (!getinoquota(ip))
			(void)chkiq(ip, -1, NOCRED, 0);
#endif
		error = VOP_TRUNCATE(vp, (off_t)0, 0, NOCRED, NULL);
		mode = ip->i_mode;
		ip->i_mode = 0;
		ip->i_rdev = 0;
		ip->i_flag |= IUPD|ICHG;
		VOP_VFREE(vp, ip->i_number, mode);
	}
	if (ip->i_flag&(IUPD|IACC|ICHG|IMOD)) {
		tv = time;
		VOP_UPDATE(vp, &tv, &tv, 0);
	}
	IUNLOCK(ip);
	ip->i_flag = 0;
	/*
	 * If we are done with the inode, reclaim it
	 * so that it can be reused immediately.
	 */
	if (vp->v_usecount == 0 && ip->i_mode == 0)
		vgone(vp);
	return (error);
}

/*
 * These macros are used to bracket UFS directory ops, so that we can
 * identify all the pages touched during directory ops which need to
 * be ordered and flushed atomically, so that they may be recovered.
 */
#define	SET_DIROP(fs) {							\
	if ((fs)->lfs_writer)						\
		tsleep(&(fs)->lfs_dirops, PRIBIO + 1, "lfs_dirop", 0);	\
	++(fs)->lfs_dirops;						\
	(fs)->lfs_doifile = 1;						\
}

#define	SET_ENDOP(fs) {							\
	--(fs)->lfs_dirops;						\
	if (!(fs)->lfs_dirops)						\
		wakeup(&(fs)->lfs_writer);				\
}

#define	MARK_VNODE(dvp)	(dvp)->v_flag |= VDIROP

int
lfs_symlink(ap)
	struct vop_symlink_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
		char *a_target;
	} */ *ap;
{
	int ret;

	SET_DIROP(VTOI(ap->a_dvp)->i_lfs);
	MARK_VNODE(ap->a_dvp);
	ret = ufs_symlink(ap);
	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs);
	return (ret);
}

int
lfs_mknod(ap)
	struct vop_mknod_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap;
{
	int ret;

	SET_DIROP(VTOI(ap->a_dvp)->i_lfs);
	MARK_VNODE(ap->a_dvp);
	ret = ufs_mknod(ap);
	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs);
	return (ret);
}

int
lfs_create(ap)
	struct vop_create_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap;
{
	int ret;

	SET_DIROP(VTOI(ap->a_dvp)->i_lfs);
	MARK_VNODE(ap->a_dvp);
	ret = ufs_create(ap);
	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs);
	return (ret);
}

int
lfs_mkdir(ap)
	struct vop_mkdir_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap;
{
	int ret;

	SET_DIROP(VTOI(ap->a_dvp)->i_lfs);
	MARK_VNODE(ap->a_dvp);
	ret = ufs_mkdir(ap);
	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs);
	return (ret);
}

int
lfs_remove(ap)
	struct vop_remove_args /* {
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap;
{
	int ret;

	SET_DIROP(VTOI(ap->a_dvp)->i_lfs);
	MARK_VNODE(ap->a_dvp);
	MARK_VNODE(ap->a_vp);
	ret = ufs_remove(ap);
	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs);
	return (ret);
}

int
lfs_rmdir(ap)
	struct vop_rmdir_args /* {
		struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap;
{
	int ret;

	SET_DIROP(VTOI(ap->a_dvp)->i_lfs);
	MARK_VNODE(ap->a_dvp);
	MARK_VNODE(ap->a_vp);
	ret = ufs_rmdir(ap);
	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs);
	return (ret);
}

int
lfs_link(ap)
	struct vop_link_args /* {
		struct vnode *a_vp;
		struct vnode *a_tdvp;
		struct componentname *a_cnp;
	} */ *ap;
{
	int ret;

	SET_DIROP(VTOI(ap->a_vp)->i_lfs);
	MARK_VNODE(ap->a_vp);
	ret = ufs_link(ap);
	SET_ENDOP(VTOI(ap->a_vp)->i_lfs);
	return (ret);
}

int
lfs_rename(ap)
	struct vop_rename_args  /* {
		struct vnode *a_fdvp;
		struct vnode *a_fvp;
		struct componentname *a_fcnp;
		struct vnode *a_tdvp;
		struct vnode *a_tvp;
		struct componentname *a_tcnp;
	} */ *ap;
{
	int ret;

	SET_DIROP(VTOI(ap->a_fdvp)->i_lfs);
	MARK_VNODE(ap->a_fdvp);
	MARK_VNODE(ap->a_tdvp);
	ret = ufs_rename(ap);
	SET_ENDOP(VTOI(ap->a_fdvp)->i_lfs);
	return (ret);
}
