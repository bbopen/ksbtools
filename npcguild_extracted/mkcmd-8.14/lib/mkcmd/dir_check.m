# $Id: dir_check.m,v 1.2 1997/08/22 02:46:33 ksb Beta $
# The directory trigger type with a verify action			(ksb)

from '<sys/stat.h>'

require "dir.m" "dir_check.mc"

augment pointer ["DIR"] type "dir" {
	verify "if (!IsDir(\"%qL\", %N)) {exit(1);}"
}
