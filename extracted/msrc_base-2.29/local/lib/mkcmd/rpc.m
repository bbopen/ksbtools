#!mkcmd
# $Id: rpc.m,v 8.30 2000/11/01 15:36:48 ksb Exp $
# Accept a program number or rpc program name, look it up as a rpc	(ksb)
# service and deep copy the rpcent structure for later use.  We use
# util_saverpcent.m.  We might take a program number not in /etc/rpc
# in the furture (LLL).
from '<sys/types.h>'
from '<sys/param.h>'
from '<sys/socket.h>'

require "util_saverpcent.m"
require "rpc.mi" "rpc.mc"

pointer ["struct rpcent"] type "rpc" {
	type update "if ((%XxB *)0 != %n) {(void)free((void *)%n);}if ((char *)0 == %N || '\\000' == %N[0]) {%n = (%XxB *)0;} else if ((%XxB *)0 == (%n = %K<rpc_cvt>1v(%N, \"%qL\"))) {exit(1);}"
	type dynamic
	type comment "%L specify a RPC program by name or number"
	type keep
	type help "locate %qp as a RPC service"
	type param "rpcname"
}

# the key that keeps us in sync with the client code -- ksb
key "rpc_cvt" 1 init {
	"rpc_cvt" "%N" "\"%qL\""
}
