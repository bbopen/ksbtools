#!mkcmd
# $Id: tftp_client.m,v 8.21 1998/11/29 01:21:12 ksb Exp $
#
# Copied for explode/mkcmd use by ksb@sa.fedex.com from the BSD
# source to tftp (see copyright in bsd.m).

from '<setjmp.h>'
from '<signal.h>'
from '<netdb.h>'

from '<stdio.h>'
from '<unistd.h>'

require "util_sigret.m" "util_errno.m" "util_tftp.m"
require "bsd.m" "tftp_client.mh" "tftp_client.mi" "tftp_client.mc"

# not sure about the init level here -- ksb
init 7 "tftp_init();"
# cleanup 7 "tftp_close();"
