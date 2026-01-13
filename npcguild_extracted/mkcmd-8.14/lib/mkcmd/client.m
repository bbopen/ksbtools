#!mkcmd
# $Id: client.m,v 1.1 1997/10/06 19:35:59 ksb Beta $
#
# This is the client side for the standard socket program.		(ksb)

require "socket.m" "util_client.m" "client.mi"

type "socket" type "client" "socket_client" {
	after '%n = %K<SockOpen>/ef;'
	help "connect to this internet end-point"
}
