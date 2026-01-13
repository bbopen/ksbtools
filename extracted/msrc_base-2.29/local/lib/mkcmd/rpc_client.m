#!mkcmd
# $Id: rpc_client.m,v 8.12 1998/09/25 14:49:11 ksb Exp $
# The remote procedure call type -- ksb
from '<rpc/rpc.h>'

require "util_errno.m"

# XXX: We should use clnt_pcreateerror(server); or some such rather than
# fprintf below -- ksb.
pointer ["CLIENT"] type "rpc_client" {
	type update "if ((%XxB *)0 != %n) {(void)clnt_destroy(%n);}if ((char*)0 == %N || '\\000' == %N[0]) {%n = (%XxB *)0;}else if ((%XxB *)0 == (%n = %ZK/ef)) {fprintf(stderr, \"%%s: %qL: %%s: %%s\\n\", %b, %N, %E);exit(1);}"
	type dynamic
	type comment "%L connect to a RPC service"
	type keep
	type help "open %qp as a RPC service"
	type param "server"
	key 1 {
		"clnt_create" "%N" "1" "1" '"tcp"'
	}
}
