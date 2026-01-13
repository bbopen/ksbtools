# $Id: cmd_echo.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
# After you include `cmd.h' you may include this tempate to provide	(ksb)
# an `echo' command to print text.  Just add
#	CMD_DEF_ECHO,
# to your table of commands.
#
%i
static int cmd_echo();
#define CMD_DEF_ECHO	{ "echo", "output text", cmd_echo, CMD_NULL, "[text]"}
%%

%c
/* echo text to the customer						(ksb)
 */
static int
cmd_echo(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register int i;
	register char *pcSep = "";

	for (i = 1; i < argc; ++i) {
		printf("%s%s", pcSep, argv[i]);
		pcSep = " ";
	}
	printf("\n");

	return 0;
}
%%
