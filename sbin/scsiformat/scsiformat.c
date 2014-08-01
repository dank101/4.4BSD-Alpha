/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
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
 *	@(#)scsiformat.c	5.2 (Berkeley) 7/20/92
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <hp300/dev/scsireg.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct scsi_inquiry inqbuf;
struct {
	int blks;
	int blksize;
} capbuf;
struct {
	struct scsi_modesense_hdr h;
	u_char p[126-12];
} msbuf;

struct scsi_fmt_cdb inq = {
	6,
	CMD_INQUIRY, 0, 0, 0, sizeof(inqbuf), 0
};
struct scsi_fmt_cdb cap = {
	10,
	CMD_READ_CAPACITY, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
struct scsi_fmt_cdb modesense = {
	6,
	CMD_MODE_SENSE, 0, 0x3f, 0, sizeof(msbuf), 0
};

int fd;
char *device;

void	do_command __P((int, struct scsi_fmt_cdb *, u_char *, int));
void	print_capacity __P((void));
void	print_inquiry __P((void));
u_char *print_mode_page __P((u_char *));
void	print_mode_sense __P((void));
void	usage __P((void));

int
main (argc, argv)
	int argc;
	char *argv[];
{
	int ch;

	while ((ch = getopt(argc, argv, "")) != EOF)
		switch(ch) {
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	device = *argv;
	if ((fd = open(device, O_RDWR, 0)) < 0) {
		(void)fprintf(stderr,
		    "scsiformat: %s: %s\n", device, strerror(errno));
		exit(1);
	}
	print_inquiry();
	print_capacity();
	print_mode_sense();
	exit(0);
}

void
print_inquiry()
{
	char idstr[32];
	int i;

	do_command(fd, &inq, (u_char *)&inqbuf, sizeof(inqbuf));
	printf("%s: ", device);

	if (inqbuf.version != 1) {
		printf("type 0x%x, qual 0x%x, ver %d\n", inqbuf.type,
			inqbuf.qual, inqbuf.version);
		return;
	}
	switch (inqbuf.type) {
	case 0:		printf("(disk)"); break;
	case 4:		printf("(WORM)"); break;
	case 5:		printf("(CD-ROM)"); break;
	case 7:		printf("(MO-DISK)"); break;
	default:	printf("(??)"); break;
	}
	bcopy((caddr_t)&inqbuf.vendor_id, (caddr_t)idstr, 28);
	for (i = 27; i > 23; --i)
		if (idstr[i] != ' ')
			break;
	idstr[i+1] = 0;
	for (i = 23; i > 7; --i)
		if (idstr[i] != ' ')
			break;
	idstr[i+1] = 0;
	for (i = 7; i >= 0; --i)
		if (idstr[i] != ' ')
			break;
	idstr[i+1] = 0;
	printf(" %s %s rev %s:", idstr, &idstr[8], &idstr[24]);
}

void
print_capacity()
{
	do_command(fd, &cap, (u_char *)&capbuf, sizeof(capbuf));
	printf(" %d blocks of %d bytes each\n", capbuf.blks, capbuf.blksize);
}

void
print_mode_sense()
{
	u_char *cp;
	u_char *ep;

	do_command(fd, &modesense, (u_char *)&msbuf, sizeof(msbuf));

	printf("\n%d bytes of mode sense data.  ", msbuf.h.len);
	printf("media type %d, %swrite protected\n", msbuf.h.media_type,
		msbuf.h.wp? "" : "not ");
	if (msbuf.h.block_desc_len) {
		printf("density 0x%x, ", msbuf.h.density);
		if (msbuf.h.number_blocks)
			printf("%d blocks of length %d\n",
				msbuf.h.number_blocks, msbuf.h.block_length);
		else
			printf("all blocks of length %d\n",
				msbuf.h.block_length);
		cp = msbuf.p;
	} else
		cp = &msbuf.h.block_desc_len + 1;

	ep = (u_char *)&msbuf + msbuf.h.len;
	while (cp < ep)
		cp = print_mode_page(cp);
}

u_char *
print_mode_page(cp)
	u_char *cp;
{
	int n = cp[1];
	int i;
	char c;

	printf("\npage type %d%s (%d bytes): ", cp[0] & 0x7f,
		(cp[0] & 0x80)? " (saveable)" : "", n);
	switch (cp[0] & 0x7f) {
	case 1:
		printf("Error Recovery parameters.\n");
		printf("\tflags = 0x%x ", i = cp[2]);
		c = '<';
		if (i & 0x80) {
			printf("%cAWRE", c);
			c = ',';
		}
		if (i & 0x40) {
			printf("%cARRE", c);
			c = ',';
		}
		if (i & 0x20) {
			printf("%cTB", c);
			c = ',';
		}
		if (i & 0x10) {
			printf("%cRC", c);
			c = ',';
		}
		if (i & 0x08) {
			printf("%cEEC", c);
			c = ',';
		}
		if (i & 0x04) {
			printf("%cPER", c);
			c = ',';
		}
		if (i & 0x02) {
			printf("%cDTE", c);
			c = ',';
		}
		if (i & 0x01) {
			printf("%cDCR", c);
			c = ',';
		}
		if (c == ',')
			printf(">");

		printf("\n\t%d retries, %d correction span bits,\n", cp[3],
			cp[4]);
		printf("\t%d head offsets, %d data strobe offsets,\n\t",
			cp[5], cp[6]);
		if (cp[7] != 0xff)
			printf("%d", cp[7]);
		else
			printf("no");
		printf(" recovery time limit.\n");
		cp += 8;
		break;

	case 2:
		printf("Disconnect/Reconnect control.\n");
		printf("\tbuffer full ratio %d, buffer empty ratio %d,\n",
			cp[2], cp[3]);
		printf("\ttime limits: %d bus inactivity, ",
			*(u_short *)&cp[4]);
		printf("%d disconnect, %d connect.\n",
			*(u_short *)&cp[6],*(u_short *)&cp[8]);
		cp += 12;
		break;

	case 3:
		{
		struct scsi_format *sf = (struct scsi_format *)cp;
		printf("Format parameters.\n");
		printf("\t%d tracks/zone, %d alt.sect./zone, ",
			sf->tracks_per_zone, sf->alt_sect_zone);
		printf("%d alt.tracks/zone,\n\t%d alt.tracks/vol., ",
			sf->alt_tracks_zone, sf->alt_tracks_vol);
		printf("%d sectors/track, %d bytes/sector, interleave %d\n",
			sf->sect_track, sf->data_sect, sf->interleave);
		printf("\ttrack skew %d, cylinder skew %d,\n",
			sf->track_skew_factor, sf->cyl_skew_factor);
		printf("\tdrive type 0x%x ", i = cp[20]);
		c = '<';
		if (i & 0x80) {
			printf("%cSSEC", c);
			c = ',';
		}
		if (i & 0x40) {
			printf("%cHSEC", c);
			c = ',';
		}
		if (i & 0x20) {
			printf("%cRMB", c);
			c = ',';
		}
		if (i & 0x10) {
			printf("%cSURF", c);
			c = ',';
		}
		if (i & 0x08) {
			printf("%cINS", c);
			c = ',';
		}
		if (i & 0x04) {
			printf("%c?", c);
			c = ',';
		}
		if (i & 0x02) {
			printf("%c?", c);
			c = ',';
		}
		if (i & 0x01) {
			printf("%c?", c);
			c = ',';
		}
		if (c == ',')
			printf(">");
		printf("\n");
		cp += 24;
		}
		break;

	case 4:
		printf("Disk Geometry parameters.\n");
		printf("\t%d cylinders, %d heads.\n",
			(cp[2] << 16) | (cp[3] << 8) | cp[4], cp[5]);
		cp += cp[1] + 2;
		break;

	default:
		printf("Unknown page type.");
		for (cp += 2, i = 0; i < n; ++i) {
			if ((i & 7) == 0)
				printf("\n\t%2d ", i);
			printf(" %02x", *cp++);
		}
		printf("\n");
		break;
	}
	return (cp);
}

void
do_command(fd, cdb, buf, len)
	int fd;
	struct scsi_fmt_cdb *cdb;
	u_char *buf;
	int len;
{
	static int on = 1;
	static int off = 0;

	if (ioctl(fd, SDIOCSFORMAT, &on) < 0) {
		(void)fprintf(stderr,
		    "scsiformat: SDIOCSFORMAT (on): %s\n", strerror(errno));
		return;
	}
	if (ioctl(fd, SDIOCSCSICOMMAND, cdb) < 0)
		(void)fprintf(stderr,
		    "scsiformat: SDIOCSCSICOMMAND: %s\n", strerror(errno));
	else if (read(fd, buf, len) < 0)
		(void)fprintf(stderr,
		    "scsiformat: read: %s\n", strerror(errno));

	if (ioctl(fd, SDIOCSFORMAT, &off) < 0)
		(void)fprintf(stderr,
		    "scsiformat: SDIOCSFORMAT (off): %s\n", strerror(errno));
}

void
usage()
{
	(void)fprintf(stderr, "usage: scsiformat device\n");
	exit(1);
}
