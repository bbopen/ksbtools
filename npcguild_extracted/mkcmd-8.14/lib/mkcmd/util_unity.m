# $Id: util_unity.m,v 1.2 1999/06/13 18:17:13 ksb Exp $
# Code to get a file back from the unity cache, has a key for the name
# of the unity round-robin record and local cache directory.
from '<setjmp.h>'
from '<signal.h>'
from '<netdb.h>'
from '<sys/stat.h>'

require "util_savehostent.m" "util_saveservent.m"
require "tftp_client.m"
require "util_unity.mh" "util_unity.mi" "util_unity.mc"

key "unity_rr" 1 init {
	'"unity.zmd.fedex.com"'  '"tftp"' '"udp"'
}

key "unity_cache" 1 init {
}

# N.B.: For the non-mkcmd programmers: use (in your main.m)
#	require "util_unity.m"
#	augment key "unity_cache" 1 {
#		'"/var/unity/tradeapp"'
#	}
# to force this module to use "tradeapp" as the cache top-level.

key "unity_fifo" 1 init {
	'"/var/run/unity"'
}
