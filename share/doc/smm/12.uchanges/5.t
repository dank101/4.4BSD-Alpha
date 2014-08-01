.\" Copyright (c) 1986 The Regents of the University of California.
.\" All rights reserved.
.\"
.\" Redistribution and use in source and binary forms, with or without
.\" modification, are permitted provided that the following conditions
.\" are met:
.\" 1. Redistributions of source code must retain the above copyright
.\"    notice, this list of conditions and the following disclaimer.
.\" 2. Redistributions in binary form must reproduce the above copyright
.\"    notice, this list of conditions and the following disclaimer in the
.\"    documentation and/or other materials provided with the distribution.
.\" 3. All advertising materials mentioning features or use of this software
.\"    must display the following acknowledgement:
.\"	This product includes software developed by the University of
.\"	California, Berkeley and its contributors.
.\" 4. Neither the name of the University nor the names of its contributors
.\"    may be used to endorse or promote products derived from this software
.\"    without specific prior written permission.
.\"
.\" THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
.\" ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
.\" IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
.\" ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
.\" FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
.\" DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
.\" OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
.\" HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
.\" LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
.\" OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
.\" SUCH DAMAGE.
.\"
.\"	@(#)5.t	6.5 (Berkeley) 10/14/90
.\"
.SH
.LG
.ce
Section 5
.SM
.sp
.PP
A new subdirectory, \fI/usr/include/protocols\fP, has been created to
keep header files that are shared between user programs and daemons.
Several header files have been moved here, including those for \fIrwhod\fP,
\fIrouted\fP, \fItimed\fP, \fIdump\fP, \fItalk\fP, and \fIrestore\fP.
.PP
Two new header files, <\fIstring.h\fP> and <\fImemory.h\fP>, have
been added for System V compatibility.
.BP disktab
Two new fields have been added to specify that the disk supports
\fIbad144\fP-style bad sector forwarding,
and that offsets should be specified by sectors rather than cylinders.
.BP dump
The header file <\fIdumprestor.h\fP> has moved
to <\fIprotocols/dumprestore.h\fP>.
.BP gettytab
New entries have been added, including a 2400 baud
dial-in rotation for modems, a 19200 baud
standard line, and an entry for the \fIxterm\fP terminal emulator
of the \fIX\fP window system.  New capabilities for automatic speed
selection and setting strict xoff/xon flow control (\fBdecctlq\fP) were added.
.BP termcap
Many new entries were added and older entries fixed.
.BP ttys
The format of the ttys file, \fI/etc/ttys\fP,
reflects the merger of information previously kept in \fI/etc/ttys\fP,
\fI/etc/securetty\fP, and \fI/etc/ttytype\fP.
The new format permits arbitrary programs, not just \fI/etc/getty\fP,
to be spawned by \fIinit\fP.
A special \fBwindow\fP field can be used to
set up a window server before spawning a terminal emulator program.
