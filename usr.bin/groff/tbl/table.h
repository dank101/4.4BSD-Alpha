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
 *	@(#)table.h	6.2 (Berkeley) 11/13/91
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>

#include <groff/cset.h>
#include <groff/cmap.h>
#include <groff/stringclass.h>
#include <groff/errarg.h>
#include <groff/error.h>
#include <groff/lib.h>

struct inc_number {
  short inc;
  short val;
};

struct entry_modifier {
  inc_number point_size;
  inc_number vertical_spacing;
  string font;
  enum { CENTER, TOP, BOTTOM } vertical_alignment;
  char zero_width;
  char stagger;

  entry_modifier();
  ~entry_modifier();
};

struct entry_format : entry_modifier {
  enum format_type {
    LEFT, 
    CENTER, 
    RIGHT, 
    NUMERIC,
    ALPHABETIC,
    SPAN, 
    VSPAN,
    HLINE,
    DOUBLE_HLINE,
    }
  type;

  entry_format(format_type);
  entry_format();
  void debug_print() const;
};

struct table_entry;
struct horizontal_span;
struct stuff;
struct vertical_rule;

class table {
  unsigned flags;
  int nrows;
  int ncolumns;
  int linesize;
  char delim[2];
  vertical_rule *vrule_list;
  stuff *stuff_list;
  horizontal_span *span_list;
  table_entry *entry_list;
  table_entry ***entry;
  char **vline;
  char *row_is_all_lines;
  string *minimum_width;
  int *column_separation;
  char *equal;
  int left_separation;
  int right_separation;
  int allocated_rows;
  void build_span_list();
  void do_hspan(int r, int c);
  void do_vspan(int r, int c);
  void allocate(int r);
  void compute_widths();
  void divide_span(int, int);
  void sum_columns(int, int);
  void compute_separation_factor();
  void compute_column_positions();
  void do_row(int);
  void init_output();
  void add_stuff(stuff *);
  void do_top();
  void do_bottom();
  void do_vertical_rules();
  void build_vrule_list();
  void add_vertical_rule(int, int, int, int);
  void define_bottom_macro();
  int vline_spanned(int r, int c);
  int row_begins_section(int);
  int row_ends_section(int);
  void make_columns_equal();
  void compute_vrule_top_adjust(int, int, string &);
  void compute_vrule_bot_adjust(int, int, string &);
  void determine_row_type();
public:
  /* used by flags */
  enum {
    CENTER = 01,
    EXPAND = 02,
    BOX = 04,
    ALLBOX = 010,
    DOUBLEBOX = 020
    };
  table(int nc, unsigned flags, int linesize);
  ~table();

  void add_text_line(int r, const string &, const char *, int);
  void add_single_hline(int r);
  void add_double_hline(int r);
  void add_entry(int r, int c, const string &, const entry_format *,
		 const char *, int lineno);
  void add_vlines(int r, const char *);
  void check();
  void print();
  void set_minimum_width(int c, const string &w);
  void set_column_separation(int c, int n);
  void set_equal_column(int c);
  void set_delim(char c1, char c2);
  void print_single_hline(int r);
  void print_double_hline(int r);
  int get_nrows();
};

void set_troff_location(const char *, int);
