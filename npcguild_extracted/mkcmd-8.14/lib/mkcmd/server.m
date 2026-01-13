#!mkcmd
# $Id: server.m,v 1.1 1997/10/06 19:35:59 ksb Beta $
#
# This is the server side for the standard socket program.		(ksb)

require "socket.m" "util_server.m" "server.mi"

type "socket" type "server" "socket_server" {
	after '%n = %K<SockBind>/ef;'
	help "bind to this internet end-point and listen"
}
