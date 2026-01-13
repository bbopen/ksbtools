#!/usr/bin/nawk
#
# $Id: GCC.awk,v 4.0 1996/03/19 15:18:49 kb207252 Beta $
#
# This file may be used with V8 awk to filter gcc-cpp output into
# something maketd will use.  The line below should be put in the
# preprocessors array in slot 0.
# "/usr/unsup/lib/gcc/gcc-cpp -M %I %F |/usr/unsup/bin/v8awk -f /usr/local/lib/GCC.awk -"
# (in some places V8 awk is nawk) (in most places it is not in "unsup" -- ksb)
BEGIN {
	 FS = "[ \t\\\\]*";
	 head = "";
}

{
	 s = 1;
}

/:/ {
	 head = $1 $2;
	 s = 3;
}

{
	for (k = s; k < NF; k++)
		if ($k !~ "^[ \t]*$")
			print head, $k;
}
