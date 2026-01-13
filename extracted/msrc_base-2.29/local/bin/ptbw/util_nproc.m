#!mkcmd
# $Id: util_nproc.m,v 1.1 2006/08/18 23:06:42 ksb Exp $
# return the number of processors online, or configured.
from '<unistd.h>'
from '<sys/types.h>'

key "util_nproc" 1 init {
	"util_nproc"
}
require "hosttype.m"
require "util_nproc.mh" "util_nproc.mi" "util_nproc.mc"
