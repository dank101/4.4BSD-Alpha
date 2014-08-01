// -*- C++ -*-
/* Copyright (C) 1991 Free Software Foundation, Inc.
     Written by James Clark (jjc@jclark.uucp)

This file is part of groff.

groff is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 1, or (at your option) any later
version.

groff is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with groff; see the file LICENSE.  If not, write to the Free Software
Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. */

/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 */

#ifndef lint
static char sccsid[] = "@(#)lkbib.cc	6.3 (Berkeley) 11/13/91";
#endif /* not lint */

#include <errno.h>
#include "refer.h"

#include "refid.h"
#include "search.h"

static void usage()
{
  fprintf(stderr, "usage: %s [-nv] [-p database] [-i XYZ] [-t N] keys ...\n",
	  program_name);
  exit(1);
}

main(int argc, char **argv)
{
  program_name = argv[0];
  static char stderr_buf[BUFSIZ];
  setbuf(stderr, stderr_buf);
  int search_default = 1;
  search_list list;
  int opt;
  while ((opt = getopt(argc, argv, "nvVi:t:p:")) != EOF)
    switch (opt) {
    case 'V':
      verify_flag = 1;
      break;
    case 'n':
      search_default = 0;
      break;
    case 'i':
      linear_ignore_fields = optarg;
      break;
    case 't':
      {
	char *ptr;
	long n = strtol(optarg, &ptr, 10);
	if (n == 0 && ptr == optarg) {
	  error("bad integer `%1' in `t' option", optarg);
	  break;
	}
	if (n < 1)
	  n = 1;
	linear_truncate_len = int(n);
	break;
      }
    case 'v':
      {
	extern const char *version_string;
	fprintf(stderr, "GNU lkbib version %s\n", version_string);
	fflush(stderr);
	break;
      }
    case 'p':
      list.add_file(optarg);
      break;
    case '?':
      usage();
    default:
      assert(0);
    }
  if (optind >= argc)
    usage();
  char *filename = getenv("REFER");
  if (filename)
    list.add_file(filename);
  else if (search_default)
    list.add_file(DEFAULT_INDEX, 1);
  if (list.nfiles() == 0)
    fatal("no databases");
  int total_len = 0;
  for (int i = optind; i < argc; i++)
    total_len += strlen(argv[i]);
  total_len += argc - optind - 1 + 1; // for spaces and '\0'
  char *buffer = new char[total_len];
  char *ptr = buffer;
  for (i = optind; i < argc; i++) {
    if (i > optind)
      *ptr++ = ' ';
    strcpy(ptr, argv[i]);
    ptr = strchr(ptr, '\0');
  }
  search_list_iterator iter(&list, buffer);
  char *start;
  int len;
  for (int count = 0; iter.next(&start, &len); count++) {
    if (fwrite(start, 1, len, stdout) != len)
      fatal("write error on stdout: %1", strerror(errno));
    // Can happen for last reference in file.
    if (start[len - 1] != '\n')
      putchar('\n');
    putchar('\n');
  }
  exit(!count);
}
