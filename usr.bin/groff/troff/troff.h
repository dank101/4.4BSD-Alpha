// -*- C++ -*-
/* Copyright (C) 1989, 1990, 1991 Free Software Foundation, Inc.
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
 * Copyright (c) 1990 The Regents of the University of California.
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
 *	@(#)troff.h	6.3 (Berkeley) 7/18/92
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <osfcn.h>
#include <errno.h>

#include <groff/lib.h>
#include <groff/assert.h>

#ifdef __GNUG__
#define NO_RETURN volatile
#else
#define NO_RETURN
#endif

NO_RETURN void cleanup_and_exit(int n);

typedef int units;

extern units scale(units n, units x, units y); // scale n by x/y

extern units units_per_inch;

extern units points_to_units(units n);
extern units scale(units, double);

extern const char *device;
extern int ascii_output_flag;
extern int suppress_output_flag;

extern int tcommand_flag;
extern int vresolution;
extern int hresolution;
extern int sizescale;

#include "groff/cset.h"
#include "groff/cmap.h"
#include "groff/errarg.h"
#include "groff/error.h"

enum warning_type {
  WARN_CHAR = 01,
  WARN_NUMBER = 02,
  WARN_BREAK = 04,
  WARN_DELIM = 010,
  WARN_EL = 020,
  WARN_SCALE = 040,
  WARN_RANGE = 0100,
  WARN_SYNTAX = 0200,
  WARN_DI = 0400,
  WARN_MAC = 01000,
  WARN_REG = 02000,
  WARN_TAB = 04000,
  WARN_RIGHT_BRACE = 010000,
  WARN_MISSING = 020000,
  WARN_INPUT = 040000,
  WARN_ESCAPE = 0100000,
  WARN_SPACE = 0200000,
  // change WARN_TOTAL if you add more warning types
};

const int WARN_TOTAL = 0377777;

int warning(warning_type, const char *,
	    const errarg & = empty_errarg,
	    const errarg & = empty_errarg,
	    const errarg & = empty_errarg);
