/*
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Sony Corp. and Kazumasa Utashiro of Software Research Associates, Inc.
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
 * from: $Hdr: diskinfo.h,v 4.300 91/06/09 06:38:01 root Rel41 $ SONY
 *
 *	@(#)diskinfo.h	7.1 (Berkeley) 6/4/92
 */

#ifndef __DISKINFO__
#define __DISKINFO__ 1

#define DISKINFO_MAGIC	0x19880427

struct Partinfo {
/*00*/	daddr_t dp_nblocks;		/* partition size (sector #) */
/*04*/	daddr_t dp_blkoff;		/* partition start block */
/*08*/
};

struct dkst {
/*00*/	int dks_ncyl;			/* # cylinders / drive */
/*04*/	int dks_ntrak;			/* # tracks / cylinder */
/*08*/	int dks_nsect;			/* # sectors / track */
/*0c*/	int dks_rps;			/* # revolutions / second */
/*10*/
};

struct diskinfo {
/*00*/	int di_magic;			/* magic number (diskinfo valid flag) */
/*04*/	struct dkst di_dkst;
/*14*/	struct Partinfo di_part[8];
/*54*/
};

struct firstsector {
/*000*/	char pad[512 - sizeof(struct diskinfo)];
/*1ac*/	struct diskinfo diskinfo;
/*200*/
};

#endif /* !__DISKINFO__ */
