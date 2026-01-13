#!mkcmd
# $Id: server.m,v 8.3 2004/12/15 22:20:18 ksb Exp $
#
# This is the server side for the standard socket program.		(ksb)

require "socket.m" "util_server.m" "server.mi"

type "socket" type "server" "socket_server" {
	after '%n = %K<SockBind>/ef;'
	help "bind to this internet end-point and listen"
}
