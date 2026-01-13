# $Id: std_xdev.m,v 8.5 1997/10/04 20:24:28 ksb Beta $
# augment the fts search to include the -x (xdev) option		(ksb)
# I think this is a good idea.  BSD find uses it, was -xdev long ago.

require "std_fts.m"

augment boolean 'R' {
	boolean 'x' {
		init '0'
		update '%n = FTS_XDEV;'
		after '%rRn |= %n;'
		help "do not cross mount points"
	}
}
