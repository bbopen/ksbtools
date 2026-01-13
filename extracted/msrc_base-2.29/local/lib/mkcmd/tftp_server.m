#!mkcmd
# $Id: tftp_server.m,v 8.20 1998/11/29 01:17:36 ksb Exp $
# Copied for explode/mkcmd use by ksb@sa.fedex.com from the BSD
# source to tftpd (see copyright in bsd.mi)

from '<setjmp.h>'
from '<signal.h>'
from '<netdb.h>'

from '<stdio.h>'
from '<unistd.h>'

require "bsd.m"
require "util_sigret.m" "util_errno.m" "util_tftp.m"
require "tftp_server.mh" "tftp_server.mi" "tftp_server.mc"

init 3 "tserv_init();"
