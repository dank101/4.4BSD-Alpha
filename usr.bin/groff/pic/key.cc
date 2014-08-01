/* C code produced by gperf version 2.3 (GNU C++ version) */
/* Command-line: gperf -p -a -t -k 1,3,4 -c -C -N lookup_keyword -T pic.gperf  */
/* Copyright (C) 1989, 1990 Free Software Foundation, Inc.
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
static char sccsid[] = "@(#)key.cc	6.2 (Berkeley) 11/13/91";
#endif /* not lint */

#include <groff/ptable.h>

#include "key.h"
#include "pic.h"
#include "object.h"
#include "pic.tab.h"

#define TOTAL_KEYWORDS 81
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 9
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 276
/* maximum key range = 275, duplicates = 0 */

static unsigned int
hash (register const char *str, register int len)
{
  static const unsigned short asso_values[] =
    {
     277, 277, 277, 277, 277, 277, 277, 277, 277, 277,
     277, 277, 277, 277, 277, 277, 277, 277, 277, 277,
     277, 277, 277, 277, 277, 277, 277, 277, 277, 277,
     277, 277, 277, 277, 277, 277, 277, 277, 277, 277,
     277, 277, 277, 277, 277, 277, 277, 277, 277, 277,
     277, 277, 277, 277, 277, 277, 277, 277, 277, 277,
     277, 277, 277, 277, 277, 277, 277, 277, 277, 277,
     277, 277,   5, 277, 277, 277, 277, 277, 277, 277,
     277, 277, 277, 277, 277, 277, 277, 277, 277, 277,
     277, 277, 277, 277, 277, 277, 277,  25,  20,  40,
      55,  35, 105,  35,  15, 110, 277, 277,  40,  95,
      25,  70,   7, 277,  30, 100,   0,  85,  20,  17,
       5,   5, 277, 277, 277, 277, 277, 277,
    };
  register int hval = len;

  switch (hval)
    {
      default:
      case 4:
        hval += asso_values[str[3]];
      case 3:
        hval += asso_values[str[2]];
      case 2:
      case 1:
        hval += asso_values[str[0]];
    }
  return hval;
}

const struct keyword *
lookup_keyword (register const char *str, register int len)
{

  static const struct keyword wordlist[] =
    {
      {"to",  TO},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"top",  TOP},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"ht",  HEIGHT},
      {"",}, {"",}, {"",}, {"",}, 
      {"by",  BY},
      {"",}, {"",}, 
      {"way",  WAY},
      {"bottom",  BOTTOM},
      {"at",  AT},
      {"box",  BOX},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"with",  WITH},
      {"",}, 
      {"the",  THE},
      {"",}, {"",}, {"",}, 
      {"cw",  CW},
      {"",}, 
      {"between",  BETWEEN},
      {"exp",  EXP},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"",}, 
      {"copy",  COPY},
      {"do",  DO},
      {"",}, {"",}, 
      {"ccw",  CCW},
      {"dotted",  DOTTED},
      {"",}, {"",}, 
      {"then",  THEN},
      {"",}, {"",}, {"",}, 
      {"arc",  ARC},
      {"",}, {"",}, 
      {"center",  CENTER},
      {"of",  OF},
      {"",}, 
      {"Here",  HERE},
      {"wid",  WIDTH},
      {"",}, 
      {"width",  WIDTH},
      {"log",  LOG},
      {"",}, 
      {"atan2",  ATAN2},
      {"plot",  PLOT},
      {"",}, 
      {"and",  AND},
      {"",}, 
      {"right",  RIGHT},
      {"",}, 
      {"up",  UP},
      {"rad",  RADIUS},
      {"",}, {"",}, {"",}, {"",}, 
      {"end",  END},
      {"",}, {"",}, {"",}, 
      {"lower",  LOWER},
      {"",}, {"",}, {"",}, 
      {"down",  DOWN},
      {"sh",  SH},
      {"max",  MAX},
      {"line",  LINE},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"if",  IF},
      {"int",  INT},
      {"rand",  RAND},
      {"",}, 
      {"circle",  CIRCLE},
      {"",}, {"",}, 
      {"thru",  THRU},
      {"above",  ABOVE},
      {"chop",  CHOP},
      {"",}, 
      {"min",  MIN},
      {"",}, {"",}, {"",}, {"",}, 
      {"sin",  SIN},
      {"",}, 
      {"arrow",  ARROW},
      {"",}, 
      {"upper",  UPPER},
      {"",}, 
      {"sqrt",  SQRT},
      {"below",  BELOW},
      {"",}, {"",}, 
      {"for",  FOR},
      {"",}, {"",}, {"",}, {"",}, 
      {"cos",  COS},
      {"last",  LAST},
      {"",}, {"",}, 
      {"print",  PRINT},
      {"",}, 
      {"left",  LEFT},
      {"",}, {"",}, {"",}, {"",}, 
      {"move",  MOVE},
      {"thick",  THICKNESS},
      {"",}, {"",}, {"",}, 
      {"thickness",  THICKNESS},
      {"start",  START},
      {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"height",  HEIGHT},
      {"",}, {"",}, {"",}, 
      {"reset",  RESET},
      {"",}, {"",}, {"",}, 
      {"else",  ELSE},
      {"",}, 
      {"dashed",  DASHED},
      {"aligned",  ALIGNED},
      {"",}, 
      {"diam",  DIAMETER},
      {"undef",  UNDEF},
      {"",}, {"",}, 
      {"diameter",  DIAMETER},
      {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"fill",  FILL},
      {"",}, 
      {"filled",  FILL},
      {"ellipse",  ELLIPSE},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"until",  UNTIL},
      {"radius",  RADIUS},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      
      {"rjust",  RJUST},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"ljust",  LJUST},
      {"",}, {"",}, {"",}, 
      {"same",  SAME},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"",}, 
      {"invis",  INVISIBLE},
      {"",}, 
      {"sprintf",  SPRINTF},
      {"",}, 
      {"invisible",  INVISIBLE},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"spline",  SPLINE},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, 
      {"from",  FROM},
      {"",}, 
      {"define",  DEFINE},
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len) - MIN_HASH_VALUE;

      if (key <= (MAX_HASH_VALUE - MIN_HASH_VALUE) && key >= 0)
        {
          register const char *s = wordlist[key].name;

          if (*s == *str && !strncmp (str + 1, s + 1, len - 1))
            return &wordlist[key];
        }
    }
  return 0;
}
