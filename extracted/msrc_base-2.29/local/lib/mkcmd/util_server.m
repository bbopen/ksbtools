#!mkcmd
# $Id: util_server.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
#
# This is the server side for the standard socket program.		(ksb)
# We open a INET socket, bind to a service/protocol,
# the fd the application programmer declares just needs a
# visible "SOCK_DESCR" structure and these 5 attributes:
#	key 2 {
#    the pointer to default address	name	SOCK_DESCR
#		'":telnet/tcp"'  	'"%qL"'	"& SDf"
#	}
#	before '%K<SockCvt>/ef;'
#	update '%K<SockUpdate>/ef;'
#	after '%n = %K<SockBind>/ef;'
#
# We can make the SOCK_DESCR structure with this C fragment:
#%c
#SOCK_DESCR SDf;
#%%
#
# If the SOCK_DESCR structure has an initial value you like, you
# do not need the before attribute at all.
#SOCK_DESCR SDf = { "", "8080" , "tcp" } ;
#
from '<sys/types.h>'
from '<sys/socket.h>'
from '<sys/param.h>'
from '<netdb.h>'
from '<netinet/in.h>'

require "util_socket.m" "util_server.mc"

key "SockBind" 2 initializer {
	"SockBind" "%ZK3ev" "%ZK2ev"
}
