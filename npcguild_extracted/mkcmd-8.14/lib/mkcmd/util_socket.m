#!mkcmd
# $Id: util_socket.m,v 8.8 2000/05/10 15:03:19 shanshan Exp $
#
# The code that describes a socket on the command line			(ksb)
# host:service/proto.  Even if you do not use the code to
# connect/listen for sockets you should build an interface
# that looks like this one.
#

require "util_socket.mc" "util_socket.mi"

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

#if !defined(HAVE_H_ERRLIST)
#define HAVE_H_ERRLIST	1
#endif

#if HAVE_H_ERRLIST
extern int h_errno;
extern char *h_errlist[];
#define hstrerror(Me)   (h_errlist[Me])
#else
#define hstrerror(Me)   "host lookup error"
#endif
#endif
%%
