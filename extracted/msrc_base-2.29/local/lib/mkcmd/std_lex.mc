/* $Id: std_lex.mc,v 8.7 2008/09/05 16:30:05 ksb Exp $
 * the idea for this module was from Allen Braunsdorf at Purdue		(ksb)
 */

%static char **%K<lex_argv>1ev%;

/* follow the list idea of which file to read				(ksb)
 */
int
yywrap()
{
	register char *pcGot;
	extern FILE *yyin;
	extern int errno;

	if (stdin == yyin) {
		clearerr(stdin);
	} else if ((FILE *)0 != yyin) {
		fclose(yyin);
	}
	yyin = (FILE *)0;
%	if ((char **)0 == %K<lex_argv>1ev || (char *)0 == *%K<lex_argv>1ev) %{
		return 1;
	}
%	pcGot = *%K<lex_argv>1ev++%;
	if ('-' == pcGot[0] && '\000' == pcGot[1]) {
		yyin = stdin;
% %H/stdin/H/pcGot/K<lex_newfile>/e<cat>
		return 0;
	}
	if ((FILE *)0 != (yyin = fopen(pcGot, "r"))) {
% %H/yyin/H/pcGot/K<lex_newfile>/e<cat>
		return 0;
	}
%	fprintf(stderr, "%%s: fopen: %%s: %%s\n", %b, pcGot, %E)%;
	return yywrap();	/* tail recursion is free! */
}

/* setup the list of files to read					(ksb)
 */
void
%%K<lex_setwrap>1ev(argc, argv)
int argc;
char **argv;
{
	if (0 == argc) {
		static char *apcArgv[] = {
%			%K<lex_argv>s/el, (char *)0
		};
%		%K<lex_argv>1ev = apcArgv%;
	} else {
%		%K<lex_argv>1ev = argv%;
	}
}
