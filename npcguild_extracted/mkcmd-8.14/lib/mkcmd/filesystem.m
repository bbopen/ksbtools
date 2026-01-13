# $Id: filesystem.m,v 1.3 1999/06/14 14:46:27 ksb Exp $
# A place holder in mkcmd 8.x for the specification like "df" takes, and
# node on a filesystem to indicate the "mount-point", so we must take and
# node on a filesystem (directory, file, device) and return a (statfs)
# structure.  Not every UNIX system has a statfs structure. :-( --ksb
from '<sys/types.h>'
from '<sys/param.h>'
from '<sys/mount.h>'
from '<sys/errno.h>'

require "filesystem.mi" "filesystem.mc"

pointer ["struct statfs"] type "filesystem" {
	type update "if ((%XxB *)0 != %n) {free((void *)%n);}if ((char*)0 == %N || '\\000' == %N[0]) {%n = (%XxB *)0;}else if ((%XxB *)0 == (%n = %K<fs_convert>1ev(%N))) {fprintf(stderr, \"%%s: %qL: %%s: %%s\\n\", %b, %N, %E);exit(1);}"
	type dynamic
	type comment "%L specifies a filesystem mount-point"
	type keep
	type help "specify %qp as a filesystem mount-point"
	type param "fs"
}

key "fs_convert" 1 init {
	"fs_convert" "%N"
}
