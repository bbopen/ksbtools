#!mkcmd
# An example command shell (captive user interface)			(ksb)
# The present thinking is that captive user interfaces are Bad because
# they limit the power of the User to the small subset of commands provided
# by the Implementor.  That's true.  You don't get the full power of the
# shell and pipe fitting tools in a CUI, you get limited choice and limited
# action.
#
# Sometimes that is what you want: limited interaction to get a simple task
# done with special purpose commands (like dbx, restore or "unrm").  This mkcmd
# interface provides the hooks to make that CUI similar to all the others
# (pwd, ls, help, and ?) but flexible enough to add the specail commands that
# would not make sence in a shell world (tracei, add *.c, or purge *.core).
#
# So here is a tiny shell that does all the template-built commands and
# /bin/ls.  Wow.

require "cmd.m" "std_help.m" "std_version.m" "example_macro.m"
require "cmd_cd.m" "cmd_echo.m" "cmd_exec.m" "cmd_exit.m" "cmd_help.m"
	"cmd_macro.m" "cmd_merge.m" "cmd_parse.m" "cmd_add.m" "cmd_shell.m"
	"cmd_source.m" "cmd_umask.m" "cmd_version.m"

# the "ls shell"
basename "lsh" ""

# Setup the new interpeter.  Read commands from stdin before we exit.
#
init 8 "cmd_init(& CSExample, CMList, sizeof(CMList)/sizeof(CMList[0]), \"example> \", 0);"
cleanup 8 "cmd_from_file(& CSExample, stdin);"

%i
static char rcsid[] =
	"$Id: example_cmd.m,v 8.8 1999/09/25 18:55:23 ksb Exp $";
%%

%c
/* you must have a "command set" structure to pass around
 */
CMDSET CSExample;

/* The list of commands in alpha order should be in a %c hunk, used only
 * in the init call (so it could be local in main if we had a way to do
 * that).
 */
CMD CMList[] = {
	CMD_DEF_ADD,
	CMD_DEF_CD,
	CMD_DEF_CHDIR,
	CMD_DEF_COMMANDS,
	CMD_DEF_COMMENT,
#if defined(CMD_DEF_DL)
	CMD_DEF_DL,
#endif
	CMD_DEF_ECHO,
	CMD_DEF_EXIT,
	CMD_DEF_HELP,
	{"ls\0/bin/ls", "list working directory", cmd_fs_exec, 0, "[names]"},
	CMD_DEF_PWD,
	CMD_DEF_QUIT,
	CMD_DEF_SHELL,
	CMD_DEF_SOURCE,
	CMD_DEF_UMASK,
	CMD_DEF_VERSION
};
%%
