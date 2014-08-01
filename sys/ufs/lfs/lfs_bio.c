/*
 * Copyright (c) 1991 Regents of the University of California.
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
 *	@(#)lfs_bio.c	7.14 (Berkeley) 8/1/92
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/resourcevar.h>
#include <sys/mount.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

/*
 * LFS block write function.
 *
 * XXX
 * No write cost accounting is done.
 * This is almost certainly wrong for synchronous operations and NFS.
 */
int	locked_queue_count;		/* XXX Count of locked-down buffers. */

int
lfs_bwrite(ap)
	struct vop_bwrite_args /* {
		struct buf *a_bp;
	} */ *ap;
{
	register struct buf *bp = ap->a_bp;
	int s;

	/*
	 * Set the delayed write flag and use reassignbuf to move the buffer
	 * from the clean list to the dirty one.
	 *
	 * Set the B_LOCKED flag and unlock the buffer, causing brelse to move
	 * the buffer onto the LOCKED free list.  This is necessary, otherwise
	 * getnewbuf() would try to reclaim the buffers using bawrite, which
	 * isn't going to work.
	 */
	if (!(bp->b_flags & B_LOCKED)) {
		++locked_queue_count;
		bp->b_flags |= B_DELWRI | B_LOCKED;
		bp->b_flags &= ~(B_READ | B_DONE | B_ERROR);
		s = splbio();
		reassignbuf(bp, bp->b_vp);
		splx(s);
	}
	brelse(bp);
	return (0);
}

/*
 * XXX
 * This routine flushes buffers out of the B_LOCKED queue when LFS has too
 * many locked down.  Eventually the pageout daemon will simply call LFS
 * when pages need to be reclaimed.  Note, we have one static count of locked
 * buffers, so we can't have more than a single file system.  To make this
 * work for multiple file systems, put the count into the mount structure.
 */
void
lfs_flush()
{
	register struct mount *mp;
	struct mount *omp;

	/* 1M in a 4K file system. */
	if (locked_queue_count < 256)
		return;
	mp = rootfs;
	do {
		/*
		 * The lock check below is to avoid races with mount
		 * and unmount.
		 */
		if (mp->mnt_stat.f_type == MOUNT_LFS &&
		    (mp->mnt_flag & (MNT_MLOCK|MNT_RDONLY|MNT_MPBUSY)) == 0 &&
		    !vfs_busy(mp)) {
			/*
			 * We set the queue to 0 here because we are about to
			 * write all the dirty buffers we have.  If more come
			 * in while we're writing the segment, they may not
			 * get written, so we want the count to reflect these
			 * new writes after the segwrite completes.
			 */
			locked_queue_count = 0;
			lfs_segwrite(mp, 0);
			omp = mp;
			mp = mp->mnt_next;
			vfs_unbusy(omp);
		} else
			mp = mp->mnt_next;
	} while (mp != rootfs);
}
