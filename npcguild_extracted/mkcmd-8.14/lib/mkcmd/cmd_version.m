# $Id: cmd_version.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
# Output the version from the tiny command interpreter.			(ksb)
# Add
#	CMD_DEF_VERSION,
# to your table of commands
#
%i
static int cmd_version();
#define CMD_DEF_VERSION	{ "version", "output a version string", cmd_version, 0 }
%%

%c
/* outputs the same string as -V, maybe another would be better?	(ksb)
 */
static int	/*ARGSUSED*/
cmd_version(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	(void)puts(rcsid);
	return 0;
}
%%
