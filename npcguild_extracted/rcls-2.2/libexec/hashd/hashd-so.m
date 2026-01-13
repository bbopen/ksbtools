#!mkcmd
# $Id: hashd-so.m,v 2.1 2000/07/27 21:42:14 ksb Exp $
# Network interface to hashd (on the garcon port by default)		(ksb)
#
# call with
#	pcHash = NetHash(achash);
# where acHash is (char [HASH_LENTH+1]), we return (char *)0 if we
# can't get one.
require "hash.mi"
require "hashd-so.mi" "hashd-so.mc"

type "client" type "hashd" "hashd_ports" {
}
type "hashd" named "fdGarcon" "pcGarcon" {
	hidden help "connection to the hashd port to get unique id"
	key 2 init {
		'"hash:garcon/tcp"'
	}
	after "/* removed for our conversion in NetHash */"
}
