#!mkcmd
# $Id: util_cache.m,v 8.32 2008/12/21 01:25:33 ksb Exp $
# Cache a file into RAM as an argv-style vector				(ksb)
# We read the file into malloc'd space and build an argv for the lines,
# which we load in and '\000' terminate.  Uncache available as well.

from '<sys/types.h>'
from '<sys/stat.h>'
from '<sys/uio.h>'
from '<string.h>'
from '<unistd.h>'

require "util_errno.m" "util_cache.mh" "util_cache.mi" "util_cache.mc"

key "cache_file" 3 init {
	"cache_file" "int fd" "unsigned *u_puLines"
}
key "cache_vector" 2 init {
	"cache_vector" "char *pcBuffer" "unsigned u_uLines"
}
key "cache_dispose" 2 init {
	"cache_dispose" "char **ppcVector"
}
