/*
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	@(#)machMon.h	7.2 (Berkeley) 2/29/92
 *
 * machMon.h --
 *
 *	Structures, constants and defines for access to the pmax prom.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * from: $Header: /sprite/src/kernel/mach/ds3100.md/RCS/machMon.h,
 *	v 9.3 90/02/20 14:34:07 shirriff Exp $ SPRITE (Berkeley)
 */

#ifndef _MACHMON
#define _MACHMON

/*
 * The prom routines use the following structure to hold strings.
 */
typedef struct {
	char	*argPtr[16];	/* Pointers to the strings. */
	char	strings[256];	/* Buffer for the strings. */
	char	*end;		/* Pointer to end of used buf. */
	int 	num;		/* Number of strings used. */
} MachStringTable;

/*
 * The prom has a jump table at the beginning of it to get to its
 * functions.
 */
#define MACH_MON_JUMP_TABLE_ADDR	0xBFC00000

/*
 * Each entry in the jump table is 8 bytes - 4 for the jump and 4 for a nop.
 */
#define MACH_MON_FUNC_ADDR(funcNum)	(MACH_MON_JUMP_TABLE_ADDR+((funcNum)*8))

/*
 * The functions:
 *
 *	MACH_MON_RESET		Run diags, check bootmode, reinit.
 *	MACH_MON_EXEC		Load new program image.
 *	MACH_MON_RESTART	Re-enter monitor command loop.
 *	MACH_MON_REINIT		Re-init monitor, then cmd loop.
 *	MACH_MON_REBOOT		Check bootmode, no config.
 *	MACH_MON_AUTOBOOT	Autoboot the system.
 *
 * The following routines access PROM saio routines and may be used by
 * standalone programs that would like to use PROM I/O:
 *
 *	MACH_MON_OPEN		Open a file.
 *	MACH_MON_READ		Read from a file.
 *	MACH_MON_WRITE		Write to a file.
 *	MACH_MON_IOCTL		Iocontrol on a file.
 *	MACH_MON_CLOSE		Close a file.
 *	MACH_MON_LSEEK		Seek on a file.
 *	MACH_MON_GETCHAR	Get character from console.
 *	MACH_MON_PUTCHAR	Put character on console.
 *	MACH_MON_SHOWCHAR	Show a char visibly.
 *	MACH_MON_GETS		gets with editing.
 *	MACH_MON_PUTS		Put string to console.
 *	MACH_MON_PRINTF		Kernel style printf to console.
 *
 * The following are other prom routines:
 *	MACH_MON_FLUSHCACHE	Flush entire cache ().
 *	MACH_MON_CLEARCACHE	Clear I & D cache in range (addr, len).
 *	MACH_MON_SAVEREGS	Save registers in a buffer.
 *	MACH_MON_LOADREGS	Get register back from buffer.
 *	MACH_MON_JUMPS8		Jump to address in s8.
 *	MACH_MON_GETENV2	Gets a string from system environment.
 *	MACH_MON_SETENV2	Sets a string in system environment.
 *	MACH_MON_ATONUM		Converts ascii string to number.
 *	MACH_MON_STRCMP		Compares strings (strcmp).
 *	MACH_MON_STRLEN		Length of string (strlen).
 *	MACH_MON_STRCPY		Copies string (strcpy).
 *	MACH_MON_STRCAT		Appends string (strcat).
 *	MACH_MON_GETCMD		Gets a command.
 *	MACH_MON_GETNUMS	Gets numbers.
 *	MACH_MON_ARGPARSE	Parses string to argc,argv.
 *	MACH_MON_HELP		Help on prom commands.
 *	MACH_MON_DUMP		Dumps memory.
 *	MACH_MON_SETENV		Sets a string in system environment.
 *	MACH_MON_UNSETENV	Unsets a string in system environment
 *	MACH_MON_PRINTENV	Prints system environment
 *	MACH_MON_JUMP2S8	Jumps to s8
 *	MACH_MON_ENABLE		Performs prom enable command.
 *	MACH_MON_DISABLE	Performs prom disable command.
 *	MACH_MON_ZEROB		Zeros a system buffer.
 */
#define MACH_MON_RESET		MACH_MON_FUNC_ADDR(0)
#define MACH_MON_EXEC		MACH_MON_FUNC_ADDR(1)
#define MACH_MON_RESTART	MACH_MON_FUNC_ADDR(2)
#define MACH_MON_REINIT		MACH_MON_FUNC_ADDR(3)
#define MACH_MON_REBOOT		MACH_MON_FUNC_ADDR(4)
#define MACH_MON_AUTOBOOT	MACH_MON_FUNC_ADDR(5)
#define MACH_MON_OPEN		MACH_MON_FUNC_ADDR(6)
#define MACH_MON_READ		MACH_MON_FUNC_ADDR(7)
#define MACH_MON_WRITE		MACH_MON_FUNC_ADDR(8)
#define MACH_MON_IOCTL		MACH_MON_FUNC_ADDR(9)
#define MACH_MON_CLOSE		MACH_MON_FUNC_ADDR(10)
#define MACH_MON_LSEEK		MACH_MON_FUNC_ADDR(11)
#define MACH_MON_GETCHAR	MACH_MON_FUNC_ADDR(12)
#define MACH_MON_PUTCHAR	MACH_MON_FUNC_ADDR(13)
#define MACH_MON_SHOWCHAR	MACH_MON_FUNC_ADDR(14)
#define MACH_MON_GETS		MACH_MON_FUNC_ADDR(15)
#define MACH_MON_PUTS		MACH_MON_FUNC_ADDR(16)
#define MACH_MON_PRINTF		MACH_MON_FUNC_ADDR(17)
#define MACH_MON_FLUSHCACHE	MACH_MON_FUNC_ADDR(28)
#define MACH_MON_CLEARCACHE	MACH_MON_FUNC_ADDR(29)
#define MACH_MON_SAVEREGS	MACH_MON_FUNC_ADDR(30)
#define MACH_MON_LOADREGS	MACH_MON_FUNC_ADDR(31)
#define MACH_MON_JUMPS8		MACH_MON_FUNC_ADDR(32)
#define MACH_MON_GETENV2	MACH_MON_FUNC_ADDR(33)
#define MACH_MON_SETENV2	MACH_MON_FUNC_ADDR(34)
#define MACH_MON_ATONUM		MACH_MON_FUNC_ADDR(35)
#define MACH_MON_STRCMP		MACH_MON_FUNC_ADDR(36)
#define MACH_MON_STRLEN		MACH_MON_FUNC_ADDR(37)
#define MACH_MON_STRCPY		MACH_MON_FUNC_ADDR(38)
#define MACH_MON_STRCAT		MACH_MON_FUNC_ADDR(39)
#define MACH_MON_GETCMD		MACH_MON_FUNC_ADDR(40)
#define MACH_MON_GETNUMS	MACH_MON_FUNC_ADDR(41)
#define MACH_MON_ARGPARSE	MACH_MON_FUNC_ADDR(42)
#define MACH_MON_HELP		MACH_MON_FUNC_ADDR(43)
#define MACH_MON_DUMP		MACH_MON_FUNC_ADDR(44)
#define MACH_MON_SETENV		MACH_MON_FUNC_ADDR(45)
#define MACH_MON_UNSETENV	MACH_MON_FUNC_ADDR(46)
#define MACH_MON_PRINTENV	MACH_MON_FUNC_ADDR(47)
#define MACH_MON_JUMP2S8	MACH_MON_FUNC_ADDR(48)
#define MACH_MON_ENABLE		MACH_MON_FUNC_ADDR(49)
#define MACH_MON_DISABLE	MACH_MON_FUNC_ADDR(50)
#define MACH_MON_ZEROB		MACH_MON_FUNC_ADDR(51)

/*
 * Functions and defines to access the monitor.
 */

#ifdef _MONFUNCS
struct Mach_MonFuncs {
	int	(*m_reset)();
	int	(*m_exec)();
	int	(*m_restart)();
	int	(*m_reinit)();
	int	(*m_reboot)();
	int	(*m_autoboot)();
	int	(*m_open)();
	int	(*m_read)();
	int	(*m_write)();
	int	(*m_ioctl)();
	int	(*m_close)();
	int	(*m_lseek)();
	int	(*m_getchar)();
	int	(*m_putchar)();
	int	(*m_showchar)();
	int	(*m_gets)();
	int	(*m_puts)();
	int	(*m_printf)();
	int	(*m_flushcache)();
	int	(*m_clearcache)();
	int	(*m_save_regs)();
	int	(*m_load_regs)();
	int	(*m_jump_s8)();
	char *	(*m_getenv2)();
	int	(*m_setenv2)();
	int	(*m_atonum)();
	int	(*m_strcmp)();
	int	(*m_strlen)();
	char *	(*m_strcpy)();
	char *	(*m_strcat)();
	int	(*m_get_cmd)();
	int	(*m_get_nums)();
	int 	(*m_argparse)();
	int	(*m_help)();
	int	(*m_dump)();
	int	(*m_setenv)();
	int	(*m_unsetenv)();
	int	(*m_printenv)();
	int	(*m_jump2_s8)();
	int	(*m_enable)();
	int	(*m_disable)();
	int	(*m_zero_buf)();
};

#ifdef DEF_MONFUNCS
struct Mach_MonFuncs mach_MonFuncs = {
	(int (*)()) MACH_MON_RESET,
	(int (*)()) MACH_MON_EXEC,
	(int (*)()) MACH_MON_RESTART,
	(int (*)()) MACH_MON_REINIT,
	(int (*)()) MACH_MON_REBOOT,
	(int (*)()) MACH_MON_AUTOBOOT,
	(int (*)()) MACH_MON_OPEN,
	(int (*)()) MACH_MON_READ,
	(int (*)()) MACH_MON_WRITE,
	(int (*)()) MACH_MON_IOCTL,
	(int (*)()) MACH_MON_CLOSE,
	(int (*)()) MACH_MON_LSEEK,
	(int (*)()) MACH_MON_GETCHAR,
	(int (*)()) MACH_MON_PUTCHAR,
	(int (*)()) MACH_MON_SHOWCHAR,
	(int (*)()) MACH_MON_GETS,
	(int (*)()) MACH_MON_PUTS,
	(int (*)()) MACH_MON_PRINTF,
	(int (*)()) MACH_MON_FLUSHCACHE,
	(int (*)()) MACH_MON_CLEARCACHE,
	(int (*)()) MACH_MON_SAVEREGS,
	(int (*)()) MACH_MON_LOADREGS,
	(int (*)()) MACH_MON_JUMPS8,
	(char *(*)()) MACH_MON_GETENV2,
	(int (*)()) MACH_MON_SETENV2,
	(int (*)()) MACH_MON_ATONUM,
	(int (*)()) MACH_MON_STRCMP,
	(int (*)()) MACH_MON_STRLEN,
	(char *(*)()) MACH_MON_STRCPY,
	(char *(*)()) MACH_MON_STRCAT,
	(int (*)()) MACH_MON_GETCMD,
	(int (*)()) MACH_MON_GETNUMS,
	(int (*)()) MACH_MON_ARGPARSE,
	(int (*)()) MACH_MON_HELP,
	(int (*)()) MACH_MON_DUMP,
	(int (*)()) MACH_MON_SETENV,
	(int (*)()) MACH_MON_UNSETENV,
	(int (*)()) MACH_MON_PRINTENV,
	(int (*)()) MACH_MON_JUMP2S8,
	(int (*)()) MACH_MON_ENABLE,
	(int (*)()) MACH_MON_DISABLE,
	(int (*)()) MACH_MON_ZEROB,
};
#else /* DEF_MONFUNCS */
extern struct Mach_MonFuncs mach_MonFuncs;
#endif /* DEF_MONFUNCS */

#define getchar()		(*mach_MonFuncs.m_getchar)()
#define gets(buf)		(*mach_MonFuncs.m_gets)(buf)
#define printf			(*mach_MonFuncs.m_printf)
#define open(name,flags)	(*mach_MonFuncs.m_open)(name,flags)
#define read(fd,buf,len)	(*mach_MonFuncs.m_read)(fd,buf,len)
#define close(fd)		(*mach_MonFuncs.m_close)(fd)
#define lseek(fd,offset,mode)	(*mach_MonFuncs.m_lseek)(fd,offset,mode)
#define strcmp(s1,s2)		(*mach_MonFuncs.m_strcmp)(s1,s2)
#define strlen(s)		(*mach_MonFuncs.m_strlen)(s)
#define bzero(buf,size)		(*mach_MonFuncs.m_zero_buf)(buf,size)
#define getenv(var)		(*mach_MonFuncs.m_getenv2)(var)
#endif /* _MONFUNCS */

/*
 * The nonvolatile ram has a flag to indicate it is usable.
 */
#define MACH_USE_NON_VOLATILE 	((char *)0xbd0000c0)
#define MACH_NON_VOLATILE_FLAG	0x02

#endif /* _MACHPROM */
