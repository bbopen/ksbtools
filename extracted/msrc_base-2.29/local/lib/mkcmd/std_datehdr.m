# $Id: std_datehdr.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
#
# This puts the build date and time in the comment header at the	(ksb)
# top of prog.c.  This is not the default because it makes all the
# prog.c's diff each other.
#
# See strftime(3) for the list of things %Y will accept.

comment "%cbuilt on %Yd %Yb %YY, at %YH:%YM:%YS %YZ"
# e.g.:  * built on 15 Feb 1994, at 16:19:01 GMT
