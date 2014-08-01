/*
 * Copyright (c) 1990, 1992 Jan-Simon Pendry
 * Copyright (c) 1992 The Regents of the University of California
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
 *
 * %sccs.redist.c.%
 *
 *	@(#)mount_kernfs.c	5.1 (Berkeley) 7/18/92
 */

#include <sys/param.h>
#include <sys/mount.h>

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch, mntflags;

	mntflags = 0;
	while ((ch = getopt(argc, argv, "F:")) != EOF)
		switch(ch) {
		case 'F':
			mntflags = atoi(optarg);
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	if (mount(MOUNT_KERNFS, argv[1], mntflags, NULL)) {
		(void)fprintf(stderr, "mount_kernfs: %s\n", strerror(errno));
		exit(1);
	}
	exit(0);
}

void
usage()
{
	(void)fprintf(stderr,
	    "usage: mount_kernfs [ -F fsoptions ] /kern mount_point\n");
	exit(1);
}
