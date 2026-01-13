#!mkcmd
# $Id: client.m,v 8.3 2004/12/15 22:20:18 ksb Exp $
#
# This is the client side for the standard socket program.		(ksb)

require "socket.m" "util_client.m" "client.mi"

type "socket" type "client" "socket_client" {
	after '%n = %K<SockOpen>/ef;'
	help "connect to this internet end-point"
}
