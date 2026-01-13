#!mkcmd
# $Id: glob_ieee.m,v 8.5 2008/10/08 12:51:28 ksb Exp $
# I don't much care for the complex interface				(ksb)
# I'll write as many "glob_*" interfaces as I can, only one of which
# can be in any program (all mate to the util_glob interface).  The
# common GNU code should be next.

from '<glob.h>'
require "glob_ieee.mc"

# [1] the call to glob, [2] the call to free, and
# [3] the error routine or NULL
key "glob_functions" 1 init {
	"glob_ieee"
	"glob_ieee_free"
	"NULL"
}

key "glob_options" 1 init {
	"GLOB_TILDE|GLOB_BRACE|GLOB_NOSORT"
}

init "/* set the global glob vertors for IEEE globbing */"
init "util_glob = %K<glob_functions>1ev;"
init "util_glob_free = %K<glob_functions>2ev;"
