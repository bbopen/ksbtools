# $Id: dir_check.m,v 8.3 2004/12/15 22:20:18 ksb Exp $
# The directory trigger type with a verify action			(ksb)

from '<sys/stat.h>'

require "dir.m" "dir_check.mc"

augment pointer ["DIR"] type "dir" {
	verify "if (!IsDir(\"%qL\", %N)) {exit(1);}"
}
