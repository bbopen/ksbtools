# $Id: mode.m,v 8.3 2004/12/15 22:20:18 ksb Exp $
# mkcmd type for a UNIX mode on the command line			(ksb)
# To be useful to all program we have to take a mode in everal formats
# and preserve the information in that symbolic mode.
#
# 1) octal mode (-m 700) with a trailing optional mode (-m 0700/0055)
#	the optioncal bits are and'd with the existing mode, then or'd
#	with the fixed part. (the default optional part is "0".
# 2) ls style (-m -rw-r--r-- or  -m drwxr-xr-x), supporting "?" for
#	optional bits (-m rw-r?-r?-) or with the / notation.
# 3) chmod style (-m g+s) which can also specify bits to turn off (-m g-r)
# 4) we keep the inode format bits in another buffer
#	if cfmt is '\000' no format was given, (iifmt is not changed),
#	else iifmt should match the type given.
from "<sys/types.h>"
from "<sys/stat.h>"
from "<string.h>"
from "<stdlib.h>"
from "<sys/param.h>"
require "mode.mh"
require "mode.mi" "mode.mc"

%ih
#if !defined(u_MKCMD_MODE_PROTO)
typedef struct u_MCnode {
	mode_t iset;		/* force to on (1)			*/
	mode_t ioptional;	/* set if you can			*/
	mode_t iclear;		/* clear if you can			*/
	mode_t iifmt;		/* the prefered type			*/
	char cfmt;		/* '-', 'c', 'b', 'd', ... or \000 for none */
} u_MODE_CVT;
#endif

%%


key "mode_cvt" 1 init {
	"mode_cvt" "%N" "%n" '"%qL"'
}

pointer ["u_MODE_CVT"] type "mode" "u_mode_init" {
	type update "%K<mode_cvt>/ef;"
	type dynamic
	type comment "a mode in either UNIX format"
	type help "a mode in octal or symbolc"
	type param "mode"
}

init 4 "u_mode_init();"
