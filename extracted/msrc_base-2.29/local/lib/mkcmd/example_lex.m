#!mkcmd
# $Id: example_lex.m,v 8.8 1997/10/14 00:53:08 ksb Beta $
# an example program to test the std_lex module --ksb
# Build a lex parser with this 5 line spec:
#	%%
#	[0-9]	{ return yytext[0] - '0'; }
#	[a-f]	{ return yytext[0] - 'a'+10; }
#	[A-F]	{ return yytext[0] - 'A'+10; }
#	\n	{ 0; }
#	.	{ return  -1; }
# mkcmd this file, compile prog.c
# the program should convert Hex digits to Decimal -- kinda.
# note is acts like a filter mostly.

from '"lex.yy.c"'

require "std_help.m" "std_lex.m"

basename "trdown" ""

cleanup "ScanEm();"

%c
void
ScanEm()
{
	register int i;

	while (0 != (i = yylex())) {
		printf("%d\n", i);
	}
}
%%
