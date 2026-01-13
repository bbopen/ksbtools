#!mkcmd
# $Id: mode_out.m,v 8.3 2004/12/15 22:20:18 ksb Exp $
# %K<mode_out>1v is a function to convert a command line mode		(ksb)
# back to a text format (9, or 10 characters plus a '\000')
require "mode_out.mh"
require "mode_out.mi" "mode_out.mc"

key "mode_out" 1 init {
	"ModeToA" "(char *)0" "%n"
}
