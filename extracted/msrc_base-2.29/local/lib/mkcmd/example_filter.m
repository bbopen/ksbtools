#!mkcmd
# $Id: example_filter.m,v 8.7 1997/10/14 00:59:11 ksb Beta $
# Example of a filter.  Just maps upper to lower case (viz. tr A-Z a-z).
#
from '<ctype.h>'

require "std_help.m" "std_filter.m"

basename "trlower" ""

%c
/* silly filter to map upper to lower, just an example			(ksb)
 */
static void
Filter(fpIn, pcFrom)
FILE *fpIn;
char *pcFrom;	/* when we find errors we use this as the source name */
{
	register int c;

	while (EOF != (c = getc(fpIn))) {
		if (isupper(c))
			c = tolower(c);
		putchar(c);
	}
}
%%
