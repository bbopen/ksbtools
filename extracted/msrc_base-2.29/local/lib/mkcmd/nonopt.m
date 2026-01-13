#!mkcmd
# $Id: nonopt.m,v 8.30 2000/11/01 15:36:48 ksb Exp $
# Code for mkcmd programs don't have any command line options		(ksb)
# This is really rare, but sometime you just want to 'explode' some of
# the mkcmd code -- the driver might be in some other langauge (viz. Java).

# In the base program require this file, in the client program run
# "mkcmd -Bnonopt.m ..."

require "nonopt.mi"

%h
/* Has no mkcmd generated command line option parser.			(ksb)
 */
%%
init 1 '/* non-option main program */'

key "getopt_switches" 8 init {
	"1" "(char *)0" "-1" "0"
}
