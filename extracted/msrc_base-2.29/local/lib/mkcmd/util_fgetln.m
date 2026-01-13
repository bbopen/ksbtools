#!mkcmd
# $Id: util_fgetln.m,v 8.9 2001/04/19 20:48:45 ksb Exp $
# Emulate the fgetln(3) routine from 4.4BSD if we don't have it.	(ksb)

require "util_fgetln.mc"
from "<stdio.h>"

%ih
extern char *fgetln(/* FILE *, size_t * */);
%%

key "fgetln_tunes" 1 init {
	"256"		# start guess for a line
	"128"		# add this much when we run out
}
