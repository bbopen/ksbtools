#!mkcmd
# $Id: util_savehostent.m,v 8.3 2004/12/15 22:20:18 ksb Exp $
# Allocate a buffer and deep copy the hostent structure for later use.	(ksb)
# We know we are eventually going to loose big (viz. when IP V6 happens).
from '<sys/types.h>'
from '<sys/param.h>'
from '<sys/socket.h>'
from '<netinet/in.h>'
from '<netdb.h>'

require "util_savehostent.mi" "util_savehostent.mc"

key "util_savehostent" 2 init {
	"util_savehostent" "%n" '"%qL"'
}
