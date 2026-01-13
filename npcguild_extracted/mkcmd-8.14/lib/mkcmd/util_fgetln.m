#!mkcmd
# $Id: util_fgetln.m,v 8.8 1999/06/14 13:27:42 ksb Exp $
# Emulate the fgetln(3) routine from 4.4BSD if we don't have it.	(ksb)

require "util_fgetln.mc"
from "<stdio.h>"

%ih
extern char *fgetln(/* FILE *, size_t * */);
%%

key "fgetln_tunes" 1 init {
	"256"		# start guess for a line
	"128"		# add this much id we run out
}
