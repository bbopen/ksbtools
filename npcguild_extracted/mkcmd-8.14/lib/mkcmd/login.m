#!mkcmd
# $Id: login.m,v 1.1 1998/09/20 18:40:04 ksb Exp $
# Accept a login or uid, look it up as in the passwd file and deep copy	(ksb)
# the it for later use.  If a (struct passwd) changes a lot we are going to
# loose big.
from '<sys/types.h>'
from '<sys/param.h>'
from '<ctype.h>'
from '<pwd.h>'

require "util_savepwent.m"
require "login.mc"

pointer ["struct passwd"] type "login" {
	type update "if ((%XxB *)0 != %n) {(void)free((void *)%n);}if ((char *)0 == %N || '\\000' == %N[0]) {%n = (%XxB *)0;} else if ((%XxB *)0 == (%n = %K<login_cvt>1v(%N, \"%qL\"))) {exit(1);}"
	type dynamic
	type comment "%L specify a login by name or uid"
	type keep
	type help "locate %qp as a login"
	type param "login"
}

# the key that keeps us in sync with the client code -- ksb
key "login_cvt" 1 init {
	"login_cvt" "%N" '"%qL"'
}
