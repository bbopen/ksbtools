#!mkcmd
#@Header@
# $Id: expladdr.m,v 2.0 2000/06/05 22:55:08 ksb Exp $
# expands the address list on a RR record into the hosts (or IPs)	(ksb)
# on the record.

require "util_savehostent.m" "util_ppm.m"
require "expladdr.c" "expladdr.h"

%i
static PPM_BUF PPMAddr;
%%

init 3 "util_ppm_init(& PPMAddr, sizeof(char *), 32);"
