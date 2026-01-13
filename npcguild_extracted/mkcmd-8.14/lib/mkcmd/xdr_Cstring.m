#!mkcmd
# $Id: xdr_Cstring.m,v 8.11 1998/09/30 12:14:10 ksb Exp $
# I looked at the Sun RPC docs to learn how to do this.  I didn't really
# copy anything because this is all mkcmd source, not just C code. -- ksb
#
# This is random code from Kevin S Braunsdorf (ksb).  It is not related
# to any work I've been _paid_ to do so I don't think you can sue me for
# anything it does or doesn't do.  Use at your own risk, expense, or issue.
#
from '<sys/param.h>'
from '<rpc/types.h>'
from '<stdio.h>'
from '<rpc/xdr.h>'

require "xdr_Cstring.mh" "xdr_Cstring.mi" "xdr_Cstring.mc"

# Same as Cargv, we can't let the Implementor change the type name to not
# match the xdr function we provide, so you only get one key.
key "Cstring" 1 init {
	"Cstring" "xdr_%K<Cstring>1ev"  "U_%K<Cstring>1eUv"
}
