#!mkcmd
# $Id: util_socket.m,v 8.11 2004/04/21 15:41:38 ksb Exp $
#
# The code that describes a socket on the command line			(ksb)
# host:service/proto.  Even if you do not use the code to
# connect/listen for sockets you should build an interface
# that looks like this one.
#
from '<sys/param.h>'

require "util_herror.m" "util_socket.mc" "util_socket.mi"

# Assumes client file option provides these attributes:
#	key 2 init {
#		'"default.host:service/proto"'
#		'"%qL"'
#		'& SDnode'
#	}
#	before '%K<SockCvt>/ef;'
#	late update "%K<SockUpdate>/ef;"
#
key "SockCvt" 2 initialize {
	"SockCvt" "%ZK3ev" '%ZK2ev' '%ZK1ev'
}
key "SockUpdate" 2 initialize {
	"%K<SockCvt>1v" "%ZK3ev" "%ZK2ev" "%N"
}

%ih
#if !defined(U_SOCK_DEF)
#define U_SOCK_DEF	1
typedef struct u_SDnode {
	char u_achost[MAXHOSTNAMELEN+1];
	char u_acport[BUFSIZ];
	char *u_pcproto;
} SOCK_DESCR;
#endif
%%
