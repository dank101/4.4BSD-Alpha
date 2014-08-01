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
 *
 *	@(#)refer.h	6.2 (Berkeley) 7/21/92
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <groff/errarg.h>
#include <groff/error.h>
#include <groff/lib.h>
#include <groff/stringclass.h>
#include <groff/cset.h>
#include <groff/cmap.h>

#define DEFAULT_INDEX DEFAULT_INDEX_DIR "/" DEFAULT_INDEX_NAME

unsigned hash_string(const char *, int);
int next_size(int);

extern string capitalize_fields;
extern string reverse_fields;
extern string abbreviate_fields;
extern string period_before_last_name;
extern string period_before_initial;
extern string period_before_hyphen;
extern string period_before_other;
extern string sort_fields;
extern int annotation_field;
extern string annotation_macro;
extern string discard_fields;
extern string articles;
extern int abbreviate_label_ranges;
extern string label_range_indicator;
extern int date_as_label;
extern string join_authors_exactly_two;
extern string join_authors_last_two;
extern string join_authors_default;
extern string separate_label_second_parts;
extern string et_al;
extern int et_al_min_elide;
extern int et_al_min_total;

extern int compatible_flag;

extern int set_label_spec(const char *);
extern int set_date_label_spec(const char *);
extern int set_short_label_spec(const char *);

extern int short_label_flag;

void clear_labels();
void command_error(const char *,
		   const errarg &arg1 = empty_errarg,
		   const errarg &arg2 = empty_errarg,
		   const errarg &arg3 = empty_errarg);

struct reference;

void compute_labels(reference **, int);
