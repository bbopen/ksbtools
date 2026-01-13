#!mkcmd
# $Id: util_saverpcent.m,v 8.31 2000/11/01 15:52:14 ksb Exp $
# Allocate a buffer and deep copy the rpcent structure for later use.	(ksb)
from '<sys/types.h>'
from '<sys/param.h>'
from '<rpc/rpc.h>'
from '<stdio.h>'

require "util_errno.m"
require "util_saverpcent.mi" "util_saverpcent.mc"

key "util_saverpcent" 2 init {
	"util_saverpcent" "%n" '"%qL"'
}
