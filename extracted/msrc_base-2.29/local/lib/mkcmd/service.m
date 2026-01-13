#!mkcmd
# Accept a service name or port number					(ksb)
# $Id: service.m,v 8.1 1999/06/13 18:10:13 ksb Exp $
from '<sys/types.h>'
from '<sys/param.h>'
from '<string.h>'
from '<ctype.h>'
from '<netdb.h>'

require "util_saveservent.m"
require "service.mc"

pointer ["struct servent"] type "service" {
	type update "if ((%XxB *)0 != %n) {(void)free((void *)%n);}if ((char *)0 == %N || '\\000' == %N[0]) {%n = (%XxB *)0;} else if ((%XxB *)0 == (%n = %K<service_cvt>1v(%N, \"%qL\"))) {exit(1);}"
	type dynamic
	type comment "%L specify a service by name or port number"
	type keep
	type help "locate %qp as a service"
	type param "service"
}

# the key that keeps us in sync with the client code -- ksb
key "service_cvt" 1 init {
	"service_cvt" "tcp"
}
