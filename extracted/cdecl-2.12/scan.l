%{
#include <ctype.h>
#include "machine.h"
#include "y.tab.h"
/* $Id: scan.l,v 2.10 2000/06/17 22:36:10 ksb Exp $
 */

extern char *visible(), *ds();
%}
N	[0-9]
A	[A-Z_a-z]
AN	[0-9A-Z_a-z]
W	[ \t\b\f]
%%
an{W}+array	|
array		{ return ARRAY; }
as		{ return AS; }
casted{W}+to	|
cast{W}+to	|
casted		|
cast		{ return CAST; }
declare		{ return DECLARE; }
explain		{ return EXPLAIN; }
a{W}+function	|
function	{ return FUNCTION; }
"?"		|
help		{ return HELP; }
into		{ return INTO; }
of		{ return OF; }
a{W}+pointer	|
pointer		{ return POINTER; }
that{W}returns	|
returning	{ return RETURNING; }
to{W}+be	|
to		{ return TO; }

a{W}+char	|
char		{ yylval.dynstr = ds(yytext); return CHAR; }
a{W}+double	|
double		{ yylval.dynstr = ds(yytext); return DOUBLE; }
an{W}+enum	|
enum		{ yylval.dynstr = ds(yytext); return STRUCTUNION; }
a{W}+float	|
float		{ yylval.dynstr = ds(yytext); return FLOAT; }
an{W}+int	|
int		{ yylval.dynstr = ds(yytext); return INT; }
a{W}+long	|
long		{ yylval.dynstr = ds(yytext); return LONG; }
a{W}+short	|
short		{ yylval.dynstr = ds(yytext); return SHORT; }
a{W}+struct	|
struct		{ yylval.dynstr = ds(yytext); return STRUCTUNION; }
a{W}+union	|
union		{ yylval.dynstr = ds(yytext); return STRUCTUNION; }
an{W}+unsigned	|
unsigned	{ yylval.dynstr = ds(yytext); return UNSIGNED; }

{A}{AN}*	{ yylval.dynstr = ds(yytext); return NAME; }
{N}+		{ yylval.dynstr = ds(yytext); return NUMBER; }

[\t ]		{ ; }
[*[\]()\n]	{ return *yytext; }
";"		{ return '\n'; }
.		{
			printf("bad character '%s'\n",visible(*yytext));
			return *yytext;
		}
%%
char *
visible(c)
{
	static char buf[5];

	c &= 0377;
	if (isprint(c)) {
		buf[0] = c;
		buf[1] = '\0';
	} else {
		sprintf(buf,"\\%02o",c);
	}
	return buf;
}
