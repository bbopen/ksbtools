#!mkcmd
# $Id: host.m,v 8.4 1999/06/14 14:33:12 ksb Exp $
# Accept a hostname or IP addess, look it up as a host and deep copy	(ksb)
# the hostent structure for later use.  We use util_savehostent.m
from '<sys/types.h>'
from '<sys/param.h>'
from '<sys/socket.h>'
from '<netinet/in.h>'
from '<string.h>'
from '<ctype.h>'
from '<netdb.h>'

require "util_savehostent.m"
require "host.mi" "host.mc"

pointer ["struct hostent"] type "host" {
	type update "if ((%XxB *)0 != %n) {(void)free((void *)%n);}if ((char *)0 == %N || '\\000' == %N[0]) {%n = (%XxB *)0;} else if ((%XxB *)0 == (%n = %K<host_cvt>1v(%N, \"%qL\"))) {exit(1);}"
	type dynamic
	type comment "%L specify a host by name or address"
	type keep
	type help "locate %qp as a host"
	type param "hostname"
}

# the key that keeps us in sync with the client code -- ksb
key "host_cvt" 1 init {
	"host_cvt" "%N" "\"%qL\""
}
