# $Id: cmd_exit.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
# After you include `cmd.h' you may include this tempate to provide	(ksb)
# an `exit' command to exit the process w/o any cleanup.  Just add
#	CMD_DEF_EXIT,
# to your table of commands.
#
%i
static int cmd_exit();
#define CMD_DEF_EXIT	{ "exit", "quit the program without any cleanup", cmd_exit, CMD_NULL|CMD_RET, "[code]"}
%%

%c
/* exit this process now						(ksb)
 */
static int
cmd_exit(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	if (2 > argc) {
		exit(1);
	}
	exit(atoi(argv[1]));
}
%%
