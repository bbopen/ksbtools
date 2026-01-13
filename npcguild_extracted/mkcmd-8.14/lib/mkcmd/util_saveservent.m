#!mkcmd
# Accept a service name or port number					(ksb)
# $Id: util_saveservent.m,v 1.1 1999/06/13 18:10:13 ksb Exp $
from '<sys/types.h>'
from '<sys/param.h>'
from '<string.h>'
from '<ctype.h>'
from '<netdb.h>'

require "util_saveservent.mi" "util_saveservent.mc"

key "util_saveservent" 1 init {
	"util_saveservent" "%n"
}
