/*
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1992 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static char sccsid[] = "@(#)mkboot.c	7.3 (Berkeley) 4/19/92";
#endif not lint

#include <stdio.h>
#include "param.h"
#include "exec.h"
#include "disklabel.h"

#include "dec_boot.h"

/* this is the size of the standard ULTRIX boot */
#define MAXBOOTSIZE (15 * DEV_BSIZE)

struct	Dec_DiskBoot decBootInfo;
char	block[DEV_BSIZE];
char	*bootfname, *xxboot, *bootxx;

/*
 * This program takes a boot program and splits it into xxboot and bootxx
 * files for the disklabel program. The disklabel program should be used to
 * label and install the boot program onto a new disk.
 *
 * mkboot bootprog xxboot bootxx
 */
main(argc, argv)
	int argc;
	char *argv[];
{
	register int i, n;
	int ifd, ofd1, ofd2;
	int nsectors;
	long loadAddr;
	long execAddr;
	long length;

	if (argc != 4)
		usage();
	bootfname = argv[1];
	xxboot = argv[2];
	bootxx = argv[3];
	ifd = open(bootfname, 0, 0);
	if (ifd < 0) {
		perror(bootfname);
		exit(1);
	}
	ofd1 = creat(xxboot, 0666);
	if (ofd1 < 0) {
	xxboot_err:
		perror(xxboot);
		exit(1);
	}
	ofd2 = creat(bootxx, 0666);
	if (ofd2 < 0) {
	bootxx_err:
		perror(bootxx);
		exit(1);
	}

	/*
	 * Check for exec header and skip to code segment.
	 */
	if (!DecHeader(ifd, &loadAddr, &execAddr, &length)) {
		fprintf(stderr, "Need impure text format (OMAGIC) file\n");
		exit(1);
	}
	if (length > MAXBOOTSIZE) {
		fprintf(stderr, "boot program is too big (%d > %d)\n",
			length, MAXBOOTSIZE);
		exit(1);
	}

	/*
	 * Write the boot information block.
	 */
	decBootInfo.magic = DEC_BOOT_MAGIC;
	decBootInfo.mode = 0;
	decBootInfo.loadAddr = loadAddr;
	decBootInfo.execAddr = execAddr;
	decBootInfo.map[0].numBlocks = nsectors =
		(length + DEV_BSIZE - 1) >> DEV_BSHIFT;
	decBootInfo.map[0].startBlock = 1;
	decBootInfo.map[1].numBlocks = 0;
	if (write(ofd1, (char *)&decBootInfo, sizeof(decBootInfo)) !=
	    sizeof(decBootInfo) || close(ofd1) != 0)
		goto xxboot_err;

	/*
	 * Write the boot code to the bootxx file.
	 */
	for (i = 0; i < nsectors && length > 0; i++) {
		if (length < DEV_BSIZE) {
			n = length;
			bzero(block, DEV_BSIZE);
		} else
			n = DEV_BSIZE;
		if (read(ifd, block, n) != n) {
			perror(bootfname);
			break;
		}
		length -= n;
		if (write(ofd2, block, DEV_BSIZE) != DEV_BSIZE) {
			perror(bootxx);
			break;
		}
	}
	if (length > 0)
		printf("Warning: didn't reach end of boot program!\n");
	exit(0);
}

usage()
{
	printf("Usage: mkboot bootprog xxboot bootxx\n");
	printf("where:\n");
	printf("\t\"bootprog\" is a -N format file < %d bytes long\n",
	       MAXBOOTSIZE);
	printf("\t\"xxboot\" is the name of the first boot block\n");
	printf("\t\"bootxx\" is the name of the remaining boot blocks.\n");
	exit(1);
}

/*
 *----------------------------------------------------------------------
 *
 * DecHeader -
 *
 *	Check if the header is a DEC (COFF) file.
 *
 * Results:
 *	Return true if all went ok.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
DecHeader(bootFID, loadAddr, execAddr, length)
	int bootFID;	/* Handle on the boot program */
	long *loadAddr;	/* Address to start loading boot program. */
	long *execAddr;	/* Address to start executing boot program. */
	long *length;	/* Length of the boot program. */
{
	struct exec aout;
	int bytesRead;

	if (lseek(bootFID, 0, 0) < 0) {
		perror(bootfname);
		return 0;
	}
	bytesRead = read(bootFID, (char *)&aout, sizeof(aout));
	if (bytesRead != sizeof(aout) || aout.ex_fhdr.magic != COFF_MAGIC ||
	    aout.a_magic != OMAGIC)
		return 0;
	*loadAddr = aout.ex_aout.codeStart;
	*execAddr = aout.a_entry;
	*length = aout.a_text + aout.a_data;
	if (lseek(bootFID, N_TXTOFF(aout), 0) < 0) {
		perror(bootfname);
		return 0;
	}
	printf("Input file is COFF format\n");
	printf("load %x, start %x, len %d\n", *loadAddr, *execAddr, *length);
	return 1;
}
