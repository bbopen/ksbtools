#!mkcmd
# $Id: util_time.m,v 8.5 1999/06/14 14:33:12 ksb Exp $
# mkcmd derived tpye for ascii times as input words.
# Base code from Victor A. Abell at Purdue University (they hold
# copy right over the convertsion code (1000 lines!)
#
# mkcmd interface my ksb, of course (ksb@sa.fedex.com)
#
# FedEx doesn't has a Free Copyright notice yet -- so we don't
# claim this is even code.  It is random bits.  I wrote it on my
# own time.  FedEx doesn't know or care about computers.
#
from '<time.h>'
require "util_time.mc" "util_time.mh" "util_time.mi"

# replace the timezone default (in minute) if you like
# I might suggest getting the timezone and putting it in a variable ...
# require "time_tz.m" to do this auto-magically
key "AtotmTz" 1 init {
	"0"
}

# call through to Vic Abell's code
key "Atotm" 1 init {
	"atotm" "%n" "%ZK1ev"
}

# ZZZ no error API yet
