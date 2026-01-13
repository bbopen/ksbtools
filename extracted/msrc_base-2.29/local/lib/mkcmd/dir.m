# $Id: dir.m,v 8.3 1997/08/22 03:23:56 ksb Beta $
# The directory trigger type -- ksb
from '<dirent.h>'

# N.B.: to get a "verfiy" that works just use "dir_check.m" rather
# than this file (or in addition to)

# really a "struct DIR *" but we force it where we need it -- ksb
#
pointer ["DIR"] type "dir" {
	type update "if ((%XxB *)0 != %n) {(void)closedir(%n);}if ((char*)0 == %N || '\\000' == %N[0]) {%n = (%XxB *)0;}else if ((%XxB *)0 == (%n = opendir(%N))) {fprintf(stderr, \"%%s: %qL: %%s: %%s\\n\", %b, %N, %E);exit(1);}"
	type dynamic
	type comment "%L opens a directory for reading"
	type keep
	type help "open %qp as a directory"
	type param "dir"
}
