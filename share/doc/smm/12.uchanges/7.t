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
.\"	@(#)7.t	6.4 (Berkeley) 10/14/90
.\"
.SH
.LG
.ce
Section 7
.SM
.sp
.BP hier
Has been updated to reflect the reorganization
to the user and system source.
.BP me
Some new macros were added:
.B \&.sm
(smaller)
and
.B \&.bu
(bulleted paragraph).
The \fIpic\fP, \fIideal\fP, and \fIgremlin\fP preprocessors are
now supported.
.BP words
Two new word lists have been add to \fI/usr/dict\fP.
The 1935 Webster's word list is available as web2
with a supplemental list in web2a.
.IP
Several hundred words have been added to \fI/usr/dict/words\fP,
both general words (``abacus, capsize, goodbye, Hispanic, ...'') and
important technical terms (all the amino acids, many mathematical
terms, a few dinosaurs, ...).
About 10 spelling errors in \fI/usr/dict/words\fP have been corrected.
.IP
Several hundred words that \fIspell\fP derives without
difficulty from existing words (\fIe.g.\fP ``getting'' from ``get''), or that
\fIspell\fP would accept anyway, \fIe.g.\fP ``1st, 2nd'' etc., have been removed
from \fI/usr/dict/words\fP.
