#!mkcmd
# $Id: mode.m,v 1.1 1999/06/14 14:33:12 ksb Exp $
# Accept a symbolic or octal mode, convert to two (ushort)s		(ksb)
from '<sys/types.h>'
from '<sys/param.h>'
from '<sys/stat.h>'
from '<string.h>'
from '<ctype.h>'

require "mode.mh" "mode.mi" "mode.mc"

# this struct tag must match the typedef decl and the key below -- ksb
pointer ["struct permbits"] type "mode" {
	type update "if ((%XxB *)0 != %n) {(void)free((void *)%n);}if ((char *)0 == %N || '\\000' == %N[0]) {%n = (%XxB *)0;} else if ((%XxB *)0 == (%n = %K<mode_cvt>/ef)) {exit(1);}"
	type dynamic
	type comment "%L specify a mode by octal value or symbolicly"
	type keep
	type help "convert %qp as a mode"
	type param "mode"
}

# the key that keeps us in sync with the client code -- ksb
key "mode_cvt" 1 init {
	"mode_cvt" "%N" '"%qL"'
}

%ih
#if !defined(MODE_PROTECT)
typedef struct permbits {
	mode_t imand;	/* bits we want */
	mode_t iopt;	/* bits we might want */
	mode_t ioff;	/* bits we force off */
	mode_t itype;	/* file type S_IFMT, or 0 */
} MODE;
#define	MODE_PROTECT	1
#endif
%%

# summary of decls for the meta C code
key "mode_struct" 1 init {
	# typedef[must match below]
	#        tag[must be in sync with details above]
	#		   mode type for st_mode in stat.h
	"MODE"	"permbits" "mode_t"
}
