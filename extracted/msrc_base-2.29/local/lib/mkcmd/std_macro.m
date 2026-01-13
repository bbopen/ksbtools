#!mkcmd
# $Id: std_macro.m,v 8.8 2008/08/16 20:48:18 ksb Exp $
# template for the ksh -o,+o,-H stuff
#
# We assume that the table of values is in the table "aMC", unless
# function o's key#1 has another array of macros in it
#

require "std_macro.mh" "std_macro.mi" "std_macro.mc"

# a block of code for the header file so the user can export the macro
# table and call it
%hi
#ifndef ENDMACRO
typedef struct MCnode {
	char *u_pcname;		/* name user sees			*/
	char *u_pccur;		/* current or initail value		*/
	char *u_pcon;		/* value set by -o macro		*/
	char *u_pcoff;		/* value set by +o macro		*/
} MACRO;
#define ENDMACRO {(char *)0, (char *)0, (char *)0, (char *)0}
#endif
%%

escape "+" function {
	named "UndoMacro"
	param "o [macro]"
	update "if ('o' != %N[0]) {%XK<macro_hooks>1v}else if ('\\000' == %N[1]) {if (EOF == %A) {ListMacros(stdout);%XK<macro_hooks>3v}}else {++%N;}%n(%N);"
	help "reset macro, or list all macros"
}

# API key, version 2 for function o
#	1 name of macro array
#	2 extern for macro array (used as a forward declaration)
#	3 name of special "failed to find" value
#	4 extern for the failed to find value
#	5 definition for failed to find value
#	6 (char *) to substitute for a (char *)0 in the value table output
function 'o' {
	key 2 initialize {
		"aMC"
		"extern MACRO %roZK1v[];"
		"u_acFail"
		"extern char %roZK3v[];"
		'char %roZK3v[] = "no such macro";'
		'"(null)"'
	}
	named "DoMacro"
	param "macro[=value]"
	update "%n(%N);"
	help "turn macro on, or assign it a value"
}

# Advanced API key to allow bad macro calls to continue or exit special
#	1 control unknown escapes (not +o)
#	2 control macros that are not found by name
#	3 exit after the "List" function (or not if you like)
# These know the name of the parameter if "pcMacro" in the function,
# and the the -o option is the master option.
key "macro_hooks" 1 init {
	# unknown +x, could call a function on %N to process other escapes
	'fprintf(stderr, "%%s: unknown escape `%%s\\\'\\n", %b, %N);exit(69);'

	# unknown macro name provided to +o
	'fprintf(stderr, "%%s: %%s: %%s\\n", %b, pcMacro, %roZK3v);exit(78);'

	# continue after list or exit?
	'exit(0);'
}
