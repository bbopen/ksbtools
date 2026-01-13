#!mkcmd
# $Id: rpc_server.m,v 8.13 1999/06/12 00:43:58 ksb Exp $
#
from '<rpc/rpc.h>'

require "util_errno.m"
require "rpc_server.mc"

cleanup 9 "%K<rpc_register>/ef;"

exit {
	named "svc_run"
	update "%n();"
}

# The name (%n) attribute is the RPC gen'd server entry point
# key values:
#	1 is the service number (like DIRPROG, or 78)
#	2 is the version number (like DIRVERS, or 1)
#	3 ... n are the transports in lowercase
function type "rpc_server" "rpc_server_list" {
	key 1 {
		"1" "1" "tcp"
	}
	help "internal type for RPC services"
}

# key values:
#	1 the name to use for the init entry point
#	2 the type of socket to get, default RPC_ANYSOCK
key "rpc_register" 1 init {
	"u_rpc_reg" "RPC_ANYSOCK"
}
