#! /bin/sh -
exec groff -Tascii -mtty ${1+"$@"}
