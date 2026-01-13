#!mkcmd
# $Id: util_client.m,v 8.8 2000/07/28 13:56:48 ksb Exp $
#
# This is the client side for the standard socket program.		(ksb)
# We open a INET socket, connect to a host:service/protocol,
# the fd the application programmer declares just needs a
# visible "SOCK_DESCR" structure and these 4 attributes:
#	before '%K<SockCvt>/ef;'
#	update '%K<SockUpdate>/ef;'
#	after '%n = %K<SockOpen>/ef;'
#	key 2 {
#		'"default.host.com:service/proto"' '"%N"' "& SFf"
#	}
#
# We can make the SOCK_DESCR structure with this C fragment:
#%c
#SOCK_DESCR SDf;
#%%
#
# If the SOCK_DESCR structure has an initial value you like, you
# do not need the before attribute at all.
#SOCK_DESCR SDf = { "localhost", "daytime" , "tcp" } ;
#
from '<sys/types.h>'
from '<sys/socket.h>'
from '<sys/param.h>'
from '<netdb.h>'
from '<netinet/in.h>'

require "util_socket.m" "util_client.mi" "util_client.mc"

key "SockOpen" 2 initialize {
	"SockOpen" "%ZK3ev" "%ZK2ev"
}
