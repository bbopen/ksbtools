#!mkcmd
# $Id: xdr_Cargv.m,v 8.11 1998/09/30 12:14:10 ksb Exp $
# I looked at the Sun RPC docs to learn how to do this.  I didn't really
# copy anything because this is all mkcmd source, not just C code. -- ksb
#
from '<sys/param.h>'
from '<rpc/types.h>'
from '<stdio.h>'
from '<rpc/xdr.h>'

require "xdr_Cargv.mh" "xdr_Cargv.mi" "xdr_Cargv.mc"

# Since the xdr/rpc thing prefixes "xdr_" to the type name we can't	(ksb)
# let the Implementor change the conversion routine name w/o changing
# the type name.  So there is just the type name and we build the
# conversion function from that string.
# You can break that if you _have_ to... sigh.
key "Cargv" 1 init {
	"Cargv" "xdr_%K<Cargv>e1v" "U_%K<Cargv>1eUv"
}
