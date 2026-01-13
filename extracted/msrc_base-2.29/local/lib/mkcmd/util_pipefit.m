#!mkcmd
# $Id: util_pipefit.m,v 8.40 2007/09/17 19:31:12 ksb Exp $
#
# Push a filter on to 0,1, or 2.  Interacts OK with <stdio.h>		(ksb)
# honor's -n, and -v (from std_control.m).
from '<sys/types.h>'
from '<sysexits.h>'
from '<unistd.h>'

require "std_control.m"
require "util_pipefit.mi" "util_pipefit.mc"

# our name and prototype
key "PipeFit" 2 init {
	"PipeFit" "const char *pcExec" "char **ppcArgv" "char **ppcEnv" "int iFd"
}

key "PipeVerbose" 2 init {
	"%rvZK1ev"	# where to send versbose messages
}
