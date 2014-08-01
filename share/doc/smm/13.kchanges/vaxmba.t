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
.\"	@(#)vaxmba.t	1.7 (Berkeley) 10/14/90
.\"
.NH
VAX MASSBUS device drivers
.PP
This section documents the modifications in the drivers for devices
on the VAX MASSBUS, with sources in \fI/sys/vaxmba\fP,
as well as general changes made to all disk and tape drivers.
.NH 2
General changes in disk drivers
.PP
Most of the disk drivers' strategy routines were changed to report
an end-of-file when attempting to read the first block after the end
of a partition.
Distinct errors are returned for nonexistent drives,
blocks out of range, and hard I/O errors.
The \fIdkblock\fP and \fIdkunit\fP macros once used to support
disk interleaving were removed, as interleaving makes no sense
with the current file system organization.
Messages for recoverable errors, such as soft ECC's,
are now handled by \fIlog\fP instead of \fIprintf\fP.
.NH 2
General changes in tape drivers
.PP
The open functions in the tape drivers now return sensible errors
if a drive is in use.
They save a pointer to the user's terminal when opened,
so that error messages from interrupt level may be printed
on the user's terminal using \fItprintf\fP.
.NH 2
Modifications to individual MASSBUS device drivers
.XP hp.c
Error recovery in the MASSBUS disk driver is considerably better now
than it was.
The driver deals with multiple errors in the same transfer
much more gracefully.
Earlier versions could go into an endless loop correcting one error,
then retrying the transfer from the beginning when a second error
was encountered.
The driver now restarts with the first sector not yet successfully
transferred.
ECC correction is now possible on bad-sector replacements.
The correct sector number is now printed in most error messages.
The code to decide whether to initiate a data transfer or
whether to do a search was corrected, and the \fIsdist/rdist\fP parameters
were split into three parameters for each drive: the minimum and maximum
rotational distances from the desired sector between which to start
a transfer, and the number of sectors to allow after a search before 
the desired sector.
The values chosen for these parameters are probably still not optimal.
.XP
There were races when doing a retry on one drive that continued with
a repositioning command (recal or seek) and when then beginning a data transfer
on another drive.
These were corrected by using a distinguished
return value, MBD_REPOSITION, from \fIhpdtint\fP to change the controller
state when reverting to positioning operations during a recovery.
The remaining steps in the recovery are then managed by \fIhpustart\fP.
Offset commands were previously done under interrupt control,
but only on the same retries as recals (every eighth retry starting
with the fourth).
They are now done on each read retry after the 16th and are done
by busy-waiting to avoid the race described above.
The tests in the error decoding section of the interrupt
handler were rearranged for clarity and to simplify the tests
for special conditions such as format operations.
The \fIhpdtint\fP times out if the drive does not become ready
after an interrupt rather than hanging at high priority.
When forwarding bad sectors, \fIhpecc\fP correctly handles partial-sector
transfers; prior versions would transfer a full sector, then continue
with a negative byte count, encountering an invalid map register immediately
thereafter.
Partial-sector transfers are requested by the virtual memory system
when swapping page tables.
.XP mba.c
The top level MASSBUS driver supports the new return code from data-transfer
interrupts that indicate a return to positioning commands before restarting
a data transfer.
It is capable of restarting a transfer after partial completion and
adjusting the starting address and byte count according to the amount
remaining.
It has also been modified to support data transfers in reverse,
required for proper error recovery on the TU78.
\fIMbustart\fP does not check drives to see that they are present,
as dual-ported disks may appear to have a type of zero
if the other port is using the disk;
in this case, the disk unit start will return MBU_BUSY.
.XP mt.c
The TU78 driver has been extensively modified and tested
to do better error recovery and to support additional operations.
