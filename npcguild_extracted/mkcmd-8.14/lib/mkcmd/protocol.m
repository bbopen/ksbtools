#!mkcmd
# Accept a protocol name or number					(ksb)
# $Id: protocol.m,v 8.0 1998/07/05 20:59:25 ksb Exp $
from '<sys/types.h>'
from '<sys/param.h>'
from '<string.h>'
from '<ctype.h>'
from '<netdb.h>'

require "protocol.mc"

pointer ["struct protoent"] type "protocol" {
	type update "if ((%XxB *)0 != %n) {(void)free((void *)%n);}if ((char *)0 == %N || '\\000' == %N[0]) {%n = (%XxB *)0;} else if ((%XxB *)0 == (%n = %K<protocol_cvt>1v(%N, \"%qL\"))) {exit(1);}"
	type dynamic
	type comment "%L specify a protocol by name or number"
	type keep
	type help "locate %qp as a protocol"
	type param "protocol"
}

# the key that keeps us in sync with the client code -- ksb
key "protocol_cvt" 1 init {
	"protocol_cvt"
}
