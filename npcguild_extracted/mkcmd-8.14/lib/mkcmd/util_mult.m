#!mkcmd
# $Id: util_mult.m,v 8.7 1997/10/10 18:29:38 ksb Beta $
#
# This template provides a standard postfix multiplier for		(ksb)
# long integer options.  To use it you must build a table and
# change the update attribute for the long option
# (unsigneds might work too).  See "seconds.m" "inches.m" and "bytes.m".

require "util_mult.mc" "util_mult.mi"

key "pm_cvt" 2 init {
	"pm_cvt" "%N" "%ZK1ev" "sizeof(%ZK1ev)/sizeof(POST_MULT)" '"%qL"'
}
key "pm_expr" 2 init {
	"pm_expr"
}
key "pm_malformed" 2 init {
	"fprintf" "stderr" '"%%s: %%s: malformed expression: %%s\\n"' "%b" "%P" "%a"
}

%hi
#if !defined(PM__TOKEN)
#define PM__TOKEN

typedef struct PMnode {		/* postfix multiplier		*/
	char ckey;		/* postfix letter		*/
	char *pctext;		/* for the help message		*/
	unsigned long umult;	/* mean times this		*/
	char *pcplural;		/* nil or plural for pctext	*/
} POST_MULT;
#endif	/* protection from multiple #includes */
%%
