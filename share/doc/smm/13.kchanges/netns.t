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
.\"	@(#)netns.t	1.4 (Berkeley) 10/14/90
.\"
.NH
Xerox Network Systems Protocols
.PP
4.3BSD now supports some of the Xerox NS protocols.
The kernel will allow the user to send or receive IDP datagrams directly,
or establish a Sequenced Packet connection.
It will generate Error Protocol packets when necessary,
and may close user connections if this is the appropriate action on
receipt of such packets.
It will respond to Echo Protocol requests.
The Routing Information Protocol is executed by a user level process,
and sufficient access has been left for other protocols to be implemented
using IDP datagrams.
It would be possible to set the additional fields required for the Packet
Exchange format at user level, to provide a daemon to respond to 
time-of-day requests, or conduct an expanding ring broadcast to
discover clearinghouses.
.PP
Wherever possible, the algorithms and data structures parallel those
used in Internet protocol support,
so that little extra effort should be required to maintain the NS protocols.
There has not yet been much effort at tuning.
.NH 2
Naming
.PP
A machine running 4.3 is allowed to have only one six-byte NS host address,
but is permitted to be on several networks.
As in the Internet case, an address of all zeros may be used to
bind the host address for an offered service.
Unlike the Internet case, an address of all zeros cannot be used
to contact a service on the same machine.  (This should be changed.)
.PP
There is only one name space of port numbers, as opposed to the Internet
case where each protocol has its own port space.
.PP
Several point-to-point connections can share the same network number.
The destination of a point-to-point connection can have a different
network number from the local end.
.PP
The files \fIns.h\fP, \fIns_pcb.h\fP, \fIns.c\fP,
\fIns_pcb.c\fP and \fIns_proto.c\fP are direct
translations of similarly named files in the netinet directory.
\fINs_pcbnotify\fP differs a little from \fIin_pcbnotify\fP in that it takes
an extra parameter which it will pass to the ``notification''
routine argument indirectly, by stuffing it in each NS control block
selected.
.PP
This header file \fIns_if.h\fP contains the declaration of the NS
variety of the per-interface address information, like \fInetinet/in_var.h\fP.
.NH 2
Encapsulations
.PP
The stipulation that each host is allowed exactly one 6 byte address
implies that each 10 Mb/s Ethernet interface other than the first
will need to reprogram its physical address.
All the 10 Mb/s Ethernet drivers supplied with 4.3BSD perform this.
The 3 Mb/s Ethernet driver does not perform any address resolution,
but uses the 6th byte of the NS host address as a PUP host number,
making it largely incompatible with altos running XNS.
In a system with both 3 Mb/s and 10 Mb/s Ethernets,
one should configure the 3 Mb/s network first.
.PP
The file \fIns_ip.c\fP contains code
providing a mechanism for sending XNS packets over any medium supporting
IP datagrams.
It builds objects that look like point-to-point interfaces
from the point of view of NS, and a protocol from the point of view of IP.
Each of these pseudo interface structures
has extra IP data at the end (a route, source and destination),
and fits exactly into an mbuf.
If the \fIifnet\fP structure grows any larger,
the extra data will have to be put in a separate mbuf,
or the whole scheme will have to be reworked more rationally.
.NH 2
Datagrams
.PP
The files \fIns_input.c\fP and \fIns_output.c\fP contain the base level
routines which interact with network interface drivers.
There is a kernel variable \fIidp_cksum\fP, which can be used to defeat
checksums for all packets.
(There ought to be an option per socket to do this).
The NS output routine manages a cached route in the protocol
control block of each socket.
If the destination has changed, the route has been marked down,
or the route was freed because of a routing change, a new route
is obtained.
The route is not used if the NS_ROUTETOIF (aka SO_DONTROUTE or MSG_DONTROUTE)
option is present.
.PP
The files \fIidp.h\fP, \fIidp_var.h\fP, and \fIidp_usrreq.c\fP are the analogues
of \fIudp.h\fP, \fIudp_var.h\fP, and \fIudp_usrreq.c\fP.
.NH 2
Error and Echo protocols
.PP
Routines for processing incoming error protocol packets
are in \fIns_error.c\fP.  They call \fIctlinput\fP routines for IDP and
SPP to maintain structural similarity to the Internet implementation.
The kernel will generate error messages indicating lack
of a listener at a port, incorrectly received checksum,
or that a packet was thrown away due to insufficient resources
at the recipient (buffer full).
The echo protocol is handled as a special case.
If there is no listener at port number 2, then the routine
that generates the ``no listener'' error message will inspect
the packet to see if it was an echo request, and if so, will
echo it.
Thus, the user is free to construct his own echoing daemon
if he so chooses.
.NH 2
Sequenced Packet Protocol
.PP
In general, this code employs the Internet TCP algorithms where possible.
By default, a three-way handshake is used in establishing connections.
There is a compile time option to employ the minimal two way
handshake.
Incoming connections may multiplexed by source machine and port, as in the
Internet case.
It will switch over ports when establishing connections if requested to do so.
.PP
The retransmission timing and strategies are much like those of TCP,
though recent performance enhancements have not yet migrated here.
There has not yet been much opportunity to tune this implementation.
The code is intended to generate keep-alive packets,
though there is some evidence this isn't working yet.
The TCP source-quench strategy hasn't been added either.
The default nominal packet size is 576 bytes, and the default
amount of buffering is 2048.
It is possible to raise both by setting appropriate socket options.
