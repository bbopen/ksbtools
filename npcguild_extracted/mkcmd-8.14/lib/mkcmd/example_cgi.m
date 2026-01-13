#!mkcmd
# $Id: example_cgi.m,v 8.7 1997/10/11 18:29:07 ksb Beta $
# Convert the CGI parameter into an argv mkcmd can use			(ksb)
# [N.B. we have to be able to write on the string given.]

require "util_cgi.m"

basename "cgi" ""

list {
	named "Exp"
	help "all params exposed as a HTML page"
}

%c
/* output our CGI arguments as we decoded them				(ksb)
 * Hmmm... should we trap <, >, &, and the like ?  Nah.
 */
int
Exp(argc, argv)
int argc;
char **argv;
{
	register int i;

	printf("Content-type: text/html\n\n");

	printf("<HTML><HEADER><TITLE>echo args</TITLE></HEAD>\n");
	printf("<BODY><PRE>\n");
	for (i = 0; i < argc; ++i) {
		printf("%2d: \"%s\"\n", i, argv[i]);
	}
	printf("</PRE></BODY>\n");
	printf("</HTML>\n");
}
%%
