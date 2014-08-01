/*
 * Copyright (c) 1989, 1991 Regents of the University of California.
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
 *	@(#)lfs_bmap.c	7.35 (Berkeley) 8/1/92
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/resourcevar.h>
#include <sys/trace.h>

#include <miscfs/specfs/specdev.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

int lfs_getlbns __P((struct vnode *, daddr_t, INDIR *, int *));

/*
 * Bmap converts a the logical block number of a file to its physical block
 * number on the disk. The conversion is done by using the logical block
 * number to index into the array of block pointers described by the dinode.
 */
int
lfs_bmap(ap)
	struct vop_bmap_args /* {
		struct vnode *a_vp;
		daddr_t  a_bn;
		struct vnode **a_vpp;
		daddr_t *a_bnp;
	} */ *ap;
{
	/*
	 * Check for underlying vnode requests and ensure that logical
	 * to physical mapping is requested.
	 */
	if (ap->a_vpp != NULL)
		*ap->a_vpp = VTOI(ap->a_vp)->i_devvp;
	if (ap->a_bnp == NULL)
		return (0);

	return (lfs_bmaparray(ap->a_vp, ap->a_bn, ap->a_bnp, NULL, NULL));
}

/*
 * LFS has a different version of bmap from FFS because of a naming conflict.
 * In FFS, meta blocks are given real disk addresses at allocation time, and
 * are linked into the device vnode, using a logical block number which is
 * the same as the physical block number.  This can't be done by LFS because
 * blocks aren't given disk addresses until they're written, so there's no
 * way to distinguish the meta-data blocks for one file from any other file.
 * This means that meta-data blocks have to be on the vnode for the file so
 * they can be found, and have to have "names" different from the standard
 * data blocks.  To do this, we divide the name space into positive and
 * negative block numbers, and give the meta-data blocks negative logical
 * numbers.  Indirect blocks are addressed by the negative address of the
 * first data block to which they point.  Double indirect blocks are addressed
 * by one less than the address of the first indirect block to which they
 * point.  Triple indirect blocks are addressed by one less than the address
 * of the first double indirect block to which they point.
 */
int
lfs_bmaparray(vp, bn, bnp, ap, nump)
	struct vnode *vp;
	register daddr_t bn;
	daddr_t *bnp;
	INDIR *ap;
	int *nump;
{
	register struct inode *ip;
	struct buf *bp;
	struct lfs *fs;
	struct vnode *devvp;
	INDIR a[NIADDR], *xap;
	daddr_t *bap, daddr;
	long metalbn;
	int error, num, off;
	struct vop_strategy_args vop_strategy_a;

	ip = VTOI(vp);
#ifdef DIAGNOSTIC
	if (ap != NULL && nump == NULL || ap == NULL && nump != NULL)
		panic("lfs_bmaparray: invalid arguments");
#endif

	xap = ap == NULL ? a : ap;
	if (!nump)
		nump = &num;
	if (error = lfs_getlbns(vp, bn, xap, nump))
		return (error);

	num = *nump;
	if (num == 0) {
		*bnp = ip->i_db[bn];
		if (*bnp == 0)
			*bnp = UNASSIGNED;
		return (0);
	}


	/* Get disk address out of indirect block array */
	daddr = ip->i_ib[xap->in_off];

	/* Fetch through the indirect blocks. */
	fs = ip->i_lfs;
	devvp = VFSTOUFS(vp->v_mount)->um_devvp;

	for (bp = NULL, ++xap; daddr && --num; ++xap) {
		/* If looking for a meta-block, break out when we find it. */
		metalbn = xap->in_lbn;
		if (metalbn == bn)
			break;

		/*
		 * Read in the appropriate indirect block.  LFS can't do a
		 * bread because bread knows that FFS will hand it the device
		 * vnode, not the file vnode, so the b_dev and b_blkno would
		 * be wrong.
		 *
		 * XXX
		 * This REALLY needs to be fixed, at the very least it needs
		 * to be rethought when the buffer cache goes away.  When it's
		 * fixed, change lfs_bmaparray and lfs_getlbns to take an ip,
		 * not a vp.
		 */
		if (bp)
			brelse(bp);
		bp = getblk(vp, metalbn, fs->lfs_bsize);
		if (bp->b_flags & (B_DONE | B_DELWRI)) {
			trace(TR_BREADHIT, pack(vp, size), metalbn);
		} else {
			trace(TR_BREADMISS, pack(vp, size), metalbn);
			bp->b_blkno = daddr;
			bp->b_flags |= B_READ;
			bp->b_dev = devvp->v_rdev;
			/*
			 * Call a strategy VOP by hand.
			 */
			vop_strategy_a.a_desc = VDESC(vop_strategy);
			vop_strategy_a.a_bp=bp;
			VOCALL(devvp->v_op, VOFFSET(vop_strategy), \
			       &vop_strategy_a);
			curproc->p_stats->p_ru.ru_inblock++;	/* XXX */
			if (error = biowait(bp)) {
				brelse(bp);
				return (error);
			}
		}
		daddr = bp->b_un.b_daddr[xap->in_off];
	}
	if (bp)
		brelse(bp);

	*bnp = daddr == 0 ? UNASSIGNED : daddr;
	return (0);
}

/*
 * Create an array of logical block number/offset pairs which represent the
 * path of indirect blocks required to access a data block.  The first "pair"
 * contains the logical block number of the appropriate single, double or
 * triple indirect block and the offset into the inode indirect block array.
 * Note, the logical block number of the inode single/double/triple indirect
 * block appears twice in the array, once with the offset into the i_ib and
 * once with the offset into the page itself.
 */
int
lfs_getlbns(vp, bn, ap, nump)
	struct vnode *vp;
	register daddr_t bn;
	INDIR *ap;
	int *nump;
{
	struct lfs *fs;
	long metalbn, realbn;
	int j, numlevels, off, sh;

	if (nump)
		*nump = 0;
	numlevels = 0;
	realbn = bn;
	if ((long)bn < 0)
		bn = -(long)bn;

	/* The first NDADDR blocks are direct blocks. */
	if (bn < NDADDR)
		return (0);

	/* 
	 * Determine the number of levels of indirection.  After this loop
	 * is done, sh indicates the number of data blocks possible at the
	 * given level of indirection, and NIADDR - j is the number of levels
	 * of indirection needed to locate the requested block.
	 */
	bn -= NDADDR;
	fs = VTOI(vp)->i_lfs;
	sh = 1;
	for (j = NIADDR; j > 0; j--) {
		sh *= NINDIR(fs);
		if (bn < sh)
			break;
		bn -= sh;
	}
	if (j == 0)
		return (EFBIG);

	/* Calculate the address of the first meta-block. */
	if (realbn >= 0)
		metalbn = -(realbn - bn + NIADDR - j);
	else
		metalbn = -(-realbn - bn + NIADDR - j);

	/* 
	 * At each iteration, off is the offset into the bap array which is
	 * an array of disk addresses at the current level of indirection.
	 * The logical block number and the offset in that block are stored
	 * into the argument array.
	 */
	++numlevels;
	ap->in_lbn = metalbn;
	ap->in_off = off = NIADDR - j;
	ap++;
	for (; j <= NIADDR; j++) {
		/* If searching for a meta-data block, quit when found. */
		if (metalbn == realbn)
			break;

		sh /= NINDIR(fs);
		off = (bn / sh) % NINDIR(fs);

		++numlevels;
		ap->in_lbn = metalbn;
		ap->in_off = off;
		++ap;

		metalbn -= -1 + off * sh;
	}
	if (nump)
		*nump = numlevels;
	return (0);
}

int
lfs_balloc(vp, iosize, lbn, bpp)
	struct vnode *vp;
	u_long iosize;
	daddr_t lbn;
	struct buf **bpp;
{
	struct buf *bp;
	struct inode *ip;
	struct lfs *fs;
	daddr_t daddr;
	int error, newblock;

	ip = VTOI(vp);
	fs = ip->i_lfs;

	/* 
	 * Three cases: it's a block beyond the end of file, it's a block in
	 * the file that may or may not have been assigned a disk address or
	 * we're writing an entire block.  Note, if the daddr is unassigned,
	 * the block might still have existed in the cache.  If it did, make
	 * sure we don't count it as a new block or zero out its contents.
	 */
	newblock = ip->i_size <= lbn << fs->lfs_bshift;
	if (!newblock && (error = VOP_BMAP(vp, lbn, NULL, &daddr)))
		return (error);

	if (newblock || daddr == UNASSIGNED || iosize == fs->lfs_bsize) {
		*bpp = bp = getblk(vp, lbn, fs->lfs_bsize);
		if (newblock ||
		    daddr == UNASSIGNED && !(bp->b_flags & B_CACHE)) {
			ip->i_blocks += btodb(fs->lfs_bsize);
			fs->lfs_bfree -= btodb(fs->lfs_bsize);
			if (iosize != fs->lfs_bsize)
				clrbuf(bp);
		}
		return (0);
	}
	return (bread(vp, lbn, fs->lfs_bsize, NOCRED, bpp));

}
