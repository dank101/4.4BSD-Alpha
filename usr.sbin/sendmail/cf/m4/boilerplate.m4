divert(-1)
#
# Copyright (c) 1983 Eric P. Allman
# Copyright (c) 1988 The Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#	This product includes software developed by the University of
#	California, Berkeley and its contributors.
# 4. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
divert(0)
######################################################################
######################################################################
#####
#####			CONFIGURATION BOILERPLATE
#####
######################################################################
######################################################################

VERSIONID(@(#)boilerplate.m4	2.9 (Berkeley) 7/20/92)

######################
#   Special macros   #
######################

# my name
Dn`MAILER'-DAEMON

# UNIX header format
DlFrom $?<$<$|$g$.  $d

# delimiter (operator) characters
Do.:%@!^/[]

# format of a total name
Dq$g$?x ($x)$.

# SMTP login message
De$j Sendmail $v/$Z ready at $b

###############
#   Options   #
###############

# (don't) preserve 8 bits on message body on input
O8False

# wait up to ten minutes for alias file rebuild
Oa10

# location of alias file
OA`'ifdef(`ALIAS_FILE', ALIAS_FILE, /etc/aliases)

# substitution for space (blank) characters
OB.

# (don't) connect to "expensive" mailers
OcFalse

# checkpoint queue runs after every 10 successful deliveries
OC10

# default delivery mode (deliver in background)
Odbackground

# (don't) automatically rebuild the alias database
ODFalse

# discard Unix-style "From_" lines at top of header
OfFalse

# temporary file mode
OF0600

# default GID
Og1

# maximum hop count
Oh17

# location of help file
OH`'ifdef(`HELP_FILE', HELP_FILE, /usr/lib/sendmail.hf)

# (don't) ignore dots as terminators in incoming messages
OiFalse

# Insist that the BIND name server be running to resolve names
OITrue

# open connection cache size
Ok2

# open connection cache timeout
OK5m

# log level
OL9

# (don't) send to me too, even in an alias expansion
OmFalse

# default messages to old style
OoTrue

# queue directory
OQ`'ifdef(`QUEUE_DIR', QUEUE_DIR, /var/spool/mqueue)

# read timeout -- violates protocols
Or2h

# queue up everything before starting transmission
OsTrue

# status file
OS`'ifdef(`STATUS_FILE', STATUS_FILE, /etc/sendmail.st)

# default timeout interval
OT3d

# time zone names (System V only)
OtPST8PDT

# default UID
Ou1

# load average at which we just queue messages
Ox8

# load average at which we refuse connections
OX12

# (don't) deliver each queued job in a separate process
OYFalse

###########################
#   Message precedences   #
###########################

Pfirst-class=0
Pspecial-delivery=100
Pbulk=-60
Pjunk=-100

#####################
#   Trusted users   #
#####################

Troot
Tdaemon
Tuucp

#########################
#   Format of headers   #
#########################

H?P?Return-Path: <$?<$<$|$g$.>
HReceived: $?sfrom $s $.by $j ($v/$Z) id $i; $b
H?D?Resent-Date: $a
H?D?Date: $a
H?F?Resent-From: $q
H?F?From: $q
H?x?Full-Name: $x
HSubject:
# HPosted-Date: $a
# H?l?Received-Date: $b
H?M?Resent-Message-Id: <$t.$i@$j>
H?M?Message-Id: <$t.$i@$j>
