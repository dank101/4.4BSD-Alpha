/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Olson.
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
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)rec_open.c	5.8 (Berkeley) 7/17/92";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <db.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include "recno.h"

DB *
__rec_open(fname, flags, mode, openinfo)
	const char *fname;
	int flags, mode;
	const RECNOINFO *openinfo;
{
	BTREE *t;
	BTREEINFO btopeninfo;
	DB *dbp;
	PAGE *h;
	struct stat sb;
	int rfd;

	/* Open the user's file -- if this fails, we're done. */
	if ((rfd = open(fname, flags, mode)) < 0)
		return (NULL);

	/* Create a btree in memory (backed by disk). */
	if (openinfo) {
		if (openinfo->flags & ~(R_FIXEDLEN|R_NOKEY|R_SNAPSHOT)) {
			errno = EINVAL;
			goto err;
		}
		btopeninfo.flags = 0;
		btopeninfo.cachesize = openinfo->cachesize;
		btopeninfo.psize = 0;
		btopeninfo.compare = NULL;
		btopeninfo.lorder = openinfo->lorder;
		dbp = __bt_open(NULL, O_RDWR, S_IRUSR | S_IWUSR, &btopeninfo);
	} else
		dbp = __bt_open(NULL, O_RDWR, S_IRUSR | S_IWUSR, NULL);
	if (dbp == NULL)
		goto err;

	/*
	 * Some fields in the tree structure are recno specific.  Fill them
	 * in and make the btree structure look like a recno structure.  We
	 * don't change the bt_ovflsize value, it's close enough and slightly
	 * bigger.
	 */
	t = dbp->internal;
	if (openinfo) {
		if (openinfo->flags & R_FIXEDLEN) {
			t->bt_flags |= BTF_FIXEDLEN;
			t->bt_reclen = openinfo->reclen;
			if (t->bt_reclen == 0) {
				errno = EINVAL;
				goto err;
			}
		}
		t->bt_bval = openinfo->bval;
	} else
		t->bt_bval = '\n';

	t->bt_flags = BTF_RECNO;
	t->bt_reof = 0;

	/*
	 * In 4.4BSD stat(2) returns true for ISSOCK on pipes.  Until
	 * then, this is fairly close.  Pipes are read-only.
	 */
	if (lseek(rfd, (off_t)0, SEEK_CUR) == -1 && errno == ESPIPE) {
		SET(t, BTF_RDONLY);
		if ((t->bt_rfp = fdopen(rfd, "r")) == NULL)
			goto err;
		t->bt_irec = ISSET(t, BTF_FIXEDLEN) ? __rec_fpipe : __rec_vpipe;
	} else {
		if (fstat(rfd, &sb))
			goto err;
		if (!(flags & (O_RDWR | O_WRONLY)))
			SET(t, BTF_RDONLY);
		if (sb.st_size > SIZE_T_MAX) {
			errno = EFBIG;
			goto err;
		}
		if ((t->bt_smap = mmap(NULL, (size_t)sb.st_size,
		    PROT_READ, 0, rfd, (off_t)0)) == (caddr_t)-1)
			goto err;
		t->bt_emap = t->bt_smap + sb.st_size;
		t->bt_rfd = rfd;
		t->bt_rfp = NULL;
		t->bt_irec = ISSET(t, BTF_FIXEDLEN) ? __rec_fmap : __rec_vmap;
	}

	/* Use the recno routines. */
	dbp->close = __rec_close;
	dbp->del = __rec_delete;
	dbp->get = __rec_get;
	dbp->put = __rec_put;
	dbp->seq = __rec_seq;
	dbp->sync = __rec_sync;

	/* If the root page was created, reset the flags. */
	if ((h = mpool_get(t->bt_mp, P_ROOT, 0)) == NULL)
		goto err;
	if ((h->flags & P_TYPE) == P_BLEAF) {
		h->flags = h->flags & ~P_TYPE | P_RLEAF;
		mpool_put(t->bt_mp, h, MPOOL_DIRTY);
	} else
		mpool_put(t->bt_mp, h, 0);

	if (openinfo && openinfo->flags & R_SNAPSHOT &&
	    t->bt_irec(t, MAX_REC_NUMBER) == RET_ERROR)
                goto err;
	return (dbp);

err:	if (dbp)
		__bt_close(dbp);
	(void)close(rfd);
	return (NULL);
}
