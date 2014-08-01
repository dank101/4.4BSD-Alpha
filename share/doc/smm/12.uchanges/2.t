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
.\"	@(#)2.t	6.7 (Berkeley) 10/14/90
.\"
.SH
.LG
.ce
Section 2
.SM
.sp
.PP
The error codes for Section 2 entries have been carefully
scrutinized to insure that the documentation properly reflects
the source code.  User-visible changes in this section lie
mostly in the area of the interprocess communication facilities;
the Xerox Network System communication procotocols have
been added and the existing communication facilities have
been extended and made more robust.
.PP
.BP adjtime
A new system call which skews the system clock
to correct the time of day.
.BP fcntl
The FASYNC option to enable the SIGIO signal
now works with sockets as well as with ttys.
The interpretation of process groups set with F_SETOWN
is the same for sockets and for ttys:
negative values refer to process groups,
positive values to processes.
This is the reverse of the previous interpretation
of socket process groups set using \fIioctl\fP to enable SIGURG. 
.BP kill
The error returned when trying to signal one's own process group
when no process group is set was changed to ESRCH.
Signal 0 can now be used as documented.
.BP lseek
Returns an ESPIPE error when seeking on sockets
(including pipes) for backward compatibility.
.BP open
When doing an open with flags O_CREAT and O_EXCL (create only if the file
did not exist), it is now considered to be an error if the target exists
and is a symbolic link, even if the symbolic link refers to a nonexistent
file.
This behavior was added for the security of programs that
need to create
files with predictable names.
.BP ptrace
A new header file, <\fIsys/ptrace.h\fP>, defines the request types.
When the process being traced stops, the parent now receives a SIGCHLD.
.BP readlink
Returns EINVAL instead of ENXIO when trying to read something other
than a symbolic link.
.BP rename
If the ISVTX (sticky text) bit is set in the mode of a directory,
files in that directory may not be the source or target of a \fIrename\fP
except by the owner of the file, the owner of the directory, or the superuser.
.BP select
Now handles more descriptors.
The mask arguments to \fIselect\fP are now treated as
pointers to arrays of integers, with the first argument determining the size
of the array.
A set of macros in
<\fIsys/types.h\fP> is provided for manipulating the file descriptor sets.
The descriptor masks are only modified when no error is returned.
.BP setsockopt
Options that could only be \fIset\fP in 4.2BSD (e.g. SO_DEBUG, SO_REUSEADDR)
can now be set or reset.  To implement this change all options must now
supply an option value which specifies if the option is to be turned on or off.
The SO_LINGER option takes a structure as its option value, including both
a boolean and an interval.
New options have been added: to get or set the amount of buffering allocated
for the socket, to get the type of the socket, and to check on error status.
Options can be set in any protocol layer that supports them;
IP, TCP and SPP all use this mechanism.
.BP setpriority
The error returned on an attempt to change another user's priority
was changed from EACCES to EPERM.
.BP setreuid
Now sets the process \fIp_uid\fP to the new effective user ID instead
of the real ID for consistency with usage elsewhere.
This avoids problems with processes that are not able
to signal themselves.
.BP sigreturn
Is a new system call designed for restoring a process' context to a
previously saved one (see \fIsetjmp/longjmp\fP).
.BP sigvec
Three new signals have been added, SIGWINCH, SIGUSR1, and
SIGUSR2.  The first is for notification of window size changes
and the other two have been reserved for users.
.BP socket
The usage of the (undocumented) SIOCSPGRP \fIioctl\fP has changed.
For consistency with \fIfcntl\fP, the argument is treated
as a process if positive and as a process group if negative.
Asynchronous I/O using SIGIO is now possible on sockets.
.BP swapon
The error returned for when requesting a device which was not
configured as a swap device was changed from ENODEV to EINVAL.
In addition, \fIswapon\fP now searches the swap device tables from
from the beginning instead of the second entry.
.BP unlink
If the ISVTX (sticky text) bit is set in the mode of a directory,
files may only be removed from that directory by the owner of the file,
the owner of the directory, or the superuser.
