#!mkcmd
# $Id: shared.m,v 1.4 2000/07/30 16:47:57 ksb Exp $
# Code for mkcmd programs that share a file with a mkcdm "main"		(ksb)
# this allows more than one parameter processor, given that one complete
# before the start of the next one (they cannot be intermingled).

# In the base program require this file, in the client program run
# "mkcmd -B shared.m ..."

# This assumes you don't change getopt_key.m's key names, or the values
# between the build of the shared objects.  That is to say it only
# works when the base program and the client module agree on the names
# of the mkcmd base code.

require "getopt_key.m"
require "shared.mi"

%h
/* Shares mkcmd base code with another module.				(ksb)
 * They must we done with "optind" before we start,  we set "progname"
 * if we share the name ( better to change that one )
 */
%%
init 1 '%K<getopt>1v(0, (char **)0, (char *)0, (char *)0);'

key "getopt_shared" 8 {
	"1" "/* shared */"
}
