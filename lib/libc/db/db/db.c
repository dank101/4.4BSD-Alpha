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

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)db.c	5.2 (Berkeley) 9/11/91";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#define	__DBINTERFACE_PRIVATE
#include <db.h>
#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include "../btree/btree.h"

DB *
dbopen(fname, flags, mode, type, openinfo)
	const char *fname;
	int flags, mode;
	DBTYPE type;
	const void *openinfo;
{
	switch (type) {
	case DB_BTREE:
		return (__bt_open(fname, flags, mode, openinfo));
	case DB_HASH:
		return (__hash_open(fname, flags, mode, openinfo));
	case DB_RECNO:
		return (__rec_open(fname, flags, mode, openinfo));
	}
	errno = EINVAL;
	return (NULL);
}

static int __db_edel __P((const DB *, const DBT *, u_int));
static int __db_eget __P((const DB *, const DBT *, DBT *, u_int));
static int __db_eput __P((const DB *dbp, const DBT *, const DBT *, u_int));
static int __db_eseq __P((const DB *, DBT *, DBT *, u_int));
static int __db_esync __P((const DB *));

/*
 * __DBPANIC -- Stop.
 *
 * Parameters:
 *	dbp:	pointer to the DB structure.
 */
void
__dbpanic(dbp)
	DB *dbp;
{
	/* The only thing that can succeed is a close. */
	dbp->del = __db_edel;
	dbp->get = __db_eget;
	dbp->put = __db_eput;
	dbp->seq = __db_eseq;
	dbp->sync = __db_esync;
}

static int
__db_edel(dbp, key, flags)
	const DB *dbp;
	const DBT *key;
	u_int flags;
{
	return (RET_ERROR);
}

static int
__db_eget(dbp, key, data, flag)
	const DB *dbp;
	const DBT *key;
	DBT *data;
	u_int flag;
{
	return (RET_ERROR);
}

static int
__db_eput(dbp, key, data, uflags)
	const DB *dbp;
	const DBT *key, *data;
	u_int uflags;
{
	return (RET_ERROR);
}

static int
__db_eseq(dbp, key, data, flags)
	const DB *dbp;
	DBT *key, *data;
	u_int flags;
{
	return (RET_ERROR);
}

static int
__db_esync(dbp)
	const DB *dbp;
{
	return (RET_ERROR);
}
