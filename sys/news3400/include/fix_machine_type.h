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
 * from: $Hdr: fix_machine_type.h,v 4.300 91/06/09 06:34:55 root Rel41 $ SONY
 *
 *	@(#)fix_machine_type.h	7.1 (Berkeley) 6/4/92
 */

#ifndef __FIX_MACHINE_TYPE__
#define __FIX_MACHINE_TYPE__

/*
 * FIX machine type definition.
 */

#if defined(news900) && !defined(news800)
#define	news800		1
#endif

#if defined(news1400) && !defined(news1700)
#define	news1700	1
#endif

#if defined(news1500) && !defined(news1700)
#define	news1700	1
#endif

#if defined(news1600) && !defined(news1700)
#define	news1700	1
#endif

#if defined(news1900) && !defined(news1800)
#define	news1800	1
#endif

#if defined(news3200) && !defined(news3400)
#define	news3400	1
#endif

#if defined(news3700) && !defined(news3400)
#define	news3400	1
#endif


/*******************************************************************************

	news700

*******************************************************************************/

#ifdef news700

#ifndef CPU_SINGLE
#define CPU_SINGLE	1
#endif

#ifdef CPU_DOUBLE
#undef CPU_DOUBLE
#endif

#ifdef IPC_3CPU
#undef IPC_3CPU
#endif

#ifdef IPC_SEMAPHORE
#undef IPC_SEMAPHORE
#endif

#ifdef IPC_RTX
#undef IPC_RTX
#endif

#ifdef IPC_MRX
#undef IPC_MRX
#endif

#endif /* news700 */

/*******************************************************************************

	news800

*******************************************************************************/

#ifdef news800

#ifdef CPU_SINGLE
#undef CPU_SINGLE
#endif

#ifndef CPU_DOUBLE
#define	CPU_DOUBLE	1
#endif

#ifdef IPC_3CPU
#undef IPC_3CPU
#endif

#ifndef IPC_SEMAPHORE
#define	IPC_SEMAPHORE	1
#endif

#ifdef IPC_RTX
#undef IPC_RTX
#endif

#ifdef IPC_MRX
#undef IPC_MRX
#endif

#endif /* news800 */

/*******************************************************************************

	news1200

*******************************************************************************/

#ifdef news1200

#ifndef CPU_SINGLE
#define	CPU_SINGLE	1
#endif

#ifdef CPU_DOUBLE
#undef CPU_DOUBLE
#endif

#ifdef IPC_3CPU
#undef IPC_3CPU
#endif

#ifdef IPC_SEMAPHORE
#undef IPC_SEMAPHORE
#endif

#ifdef IPC_RTX
#undef IPC_RTX
#endif

#ifdef IPC_MRX
#undef IPC_MRX
#endif

#endif /* news1200 */

/*******************************************************************************

	news1700

*******************************************************************************/

#ifdef news1700

#ifndef CPU_SINGLE
#define	CPU_SINGLE	1
#endif /* !CPU_SINGLE */

#ifdef CPU_DOUBLE
#undef CPU_DOUBLE
#endif /* CPU_DOUBLE */

#ifdef IPC_3CPU
#undef IPC_3CPU
#endif /* IPC_3CPU */

#ifdef IPC_SEMAPHORE
#undef IPC_SEMAPHORE
#endif /* IPC_SEMAPHORE */

#ifdef IPC_RTX
#undef IPC_RTX
#endif

#ifdef IPC_MRX
#undef IPC_MRX
#endif

#endif /* news1700 */

/*******************************************************************************

	news1800

*******************************************************************************/

#ifdef news1800

#ifdef CPU_SINGLE
#undef CPU_SINGLE
#endif

#ifndef CPU_DOUBLE
#define	CPU_DOUBLE	1
#endif

#ifdef IPC_3CPU
#undef IPC_3CPU
#endif

#ifdef IPC_SEMAPHORE
#undef IPC_SEMAPHORE
#endif

#ifdef IPC_RTX
#undef IPC_RTX
#endif

#ifndef IPC_MRX
#define IPC_MRX		1
#endif

#endif /* news1800 */

/*******************************************************************************

	news3400

*******************************************************************************/

#ifdef news3400

#ifndef PARK2_BUG
#define	PARK2_BUG
#endif

#ifndef CPU_SINGLE
#define	CPU_SINGLE	1
#endif

#ifdef CPU_DOUBLE
#undef CPU_DOUBLE
#endif

#ifdef IPC_3CPU
#undef IPC_3CPU
#endif

#ifdef IPC_SEMAPHORE
#undef IPC_SEMAPHORE
#endif

#ifdef IPC_RTX
#undef IPC_RTX
#endif

#ifdef IPC_MRX
#undef IPC_MRX
#endif

#endif /* news3400 */

/*******************************************************************************

	news3800

*******************************************************************************/

#ifdef news3800

#ifdef CPU_SINGLE
#undef CPU_SINGLE
#endif

#ifndef CPU_DOUBLE
#define	CPU_DOUBLE	1
#endif

#ifndef IPC_3CPU
#define IPC_3CPU	1
#endif

#ifdef IPC_SEMAPHORE
#undef IPC_SEMAPHORE
#endif

#ifdef IPC_RTX
#undef IPC_RTX
#endif

#ifndef IPC_MRX
#define	IPC_MRX		1
#endif

#endif /* news3800 */

#endif /* !__FIX_MACHINE_TYPE__ */
