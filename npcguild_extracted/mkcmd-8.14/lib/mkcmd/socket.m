#!mkcmd
# $Id: socket.m,v 1.2 1997/10/06 20:02:06 ksb Beta $
#
# The code that describes a socket on the command line			(ksb)
# host:service/proto.  Even if you do not use the code to
# connect/listen for sockets you should build an interface
# that looks like this one.
#

require "util_socket.m" "socket.mh" "socket.mi"

fd type "socket"  "socket_generic" {
	key 2 init {
		""		# user fill in '"default.host:service/proto"'
		'"%qL"'
		'& u_SD%n'
	}
	before "%K<SockCvt>/ef;"
	late update "%K<SockUpdate>/ef;"
	param "inet"
	help "specify network end point for communication"
}
