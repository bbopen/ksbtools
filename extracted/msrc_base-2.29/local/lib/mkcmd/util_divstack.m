#!mkcmd
# $Id: util_divstack.m,v 1.7 2008/08/25 14:17:52 ksb Exp $
# Implement a stack of diversions for ptbw, xclate, and mpmw	(ksb)
# depends on setenv(3c) and unsetenv(3c)
from '<string.h>'
from '<sys/stat.h>'
from '<sys/types.h>'
from '<sys/socket.h>'

require "util_errno.m"
require "util_divstack.mi" "util_divstack.mc"

key "divClient" 1 init {	# params to divInit
	0	# (char *) prefix we use to manage our diversions
	0	# escape for trigger which directly selects a tag
}
# e.g. to set "ptbw" with optiotn 't' use the mkcmd spec
#	key "divClient" 1 { '"ptbw"' "<respect>t" }
#
init 3 "%K<divInit>1ev(%K<divClient>/e1os/i~, \"%q~/+~L\"~/el);"

number {
	once named "iDevDepth"
	init '0'
	param "depth"
	help "select an outer diversion, depth steps away"
}

# Don't change these unless you have a really good reason.  OK I mean never.
# they are documented in sevral programs as "common convention".	(ksb)
# N.B. we must protect the printf percent escape from the mkcmd expander.
key "divLabels" 1 init {
	"link"		# the top of current wrapper stack ($xclate_link)
	"list"		# any client results go here (e.g. $ptbw_list)
	"d"		# the last detached diversion (ptbw -d set $ptbw_d)
	"%%u"		# each stack element ($xclate_2)
}

# output the ksb -V line for this module
key "divVersion" 1 init {
	"divVersion" "FILE *fpOut"
}

# Setup the divStack, you can only have one active at a time, since it is
# coded in mkcmd at compile-time.  Param1 is the prefix, param2 is the name
# of an option that forces a literal diversion tag (not a level), think of
# xclate -t or ptbw -N.
key "divInit" 1 init {
	"divInit" "const char *pcName" "const char *pcSpec"
}
key "divCurrent" 1 init {	# get the current diversion, default to Force
	"divCurrent" "char *pcForce"
}
key "divNumber" 1 init {	# current diversion as a number, or 0
	"divNumber" "unsigned *puFound"
}
key "divPush" 1 init {		# add a new diversion on the stack
	"divPush" "const char *pcNew"
}
key "divDetach" 1 init {	# add a new diversion, not on the stack
	"divDetach" "const char *pcNew" "const char *pcTag"
}
key "divSearch" 1 init {	# itteration like explode would use
	"divSearch" "int (*pfi)(const char *pcName, const char *pcDirect, int fActive, void *pvData)" "void *pvData"
}
key "divResults" 1 init {	# set the results $variable
	"divResults" "const char *pcValue"
}
key "divSelect" 1 init {
	"divSelect"
}
key "divIndex" 1 init {
	"divIndex" "unsigned uLevel"
}

# internal use only (I hope)
key "div_setKey" 1 init {
	"div_setKey" "const char *pcKey"
}
