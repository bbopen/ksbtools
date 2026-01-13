# $Id: example_macro.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
# show how to use mkcmd's -o, +o macro facility				(ksb)
#
# Macros are like environment variables, but they pack into a command
# line so you can find them all under one name.  The Implementor sees
# an interface like "getenv" (viz. Macro("name");).  The User sees an
# interface like ksh's -o macro=value that they can put in a single variable
# or maybe a config file (TODO list).
#
# Macros have a name, start value, -o value and +o value.  They are kept
# in a MACRO table called "aMC" {or set the API key on -o for another name}.
#
# You can call
#	DoMacro("name=value");
# yourself if you need to -- as well as
#	UnDoMacro("name");
# if you must. -- ksb

require "std_macro.m" "std_help.m"

basename "macro" ""

%c
MACRO aMC[] = {
	/* name	    start   set       reset */
	{"joints", "bevel", "square", "miter"},
	{"paper", "8x11", "A4", (char *)0},
	{"fruit", (char *)0, "apple", "orange"},
	ENDMACRO
};
%%

cleanup 'if ((char *)0 != Macro("fruit")) {printf("%%s: fruit is %%s\\n", %b, Macro("fruit"));}'
