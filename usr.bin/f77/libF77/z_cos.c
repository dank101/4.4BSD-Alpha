/*-
 * Copyright (c) 1980 The Regents of the University of California.
 * All rights reserved.
 *
 * This module is believed to contain source code proprietary to AT&T.
 * Use and redistribution is subject to the Berkeley Software License
 * Agreement and your Software Agreement with AT&T (Western Electric).
 */

#ifndef lint
static char sccsid[] = "@(#)z_cos.c	5.2 (Berkeley) 4/12/91";
#endif /* not lint */

#include "complex"

z_cos(r, z)
dcomplex *r, *z;
{
double sin(), cos(), sinh(), cosh();

r->dreal = cos(z->dreal) * cosh(z->dimag);
r->dimag = - sin(z->dreal) * sinh(z->dimag);
}
