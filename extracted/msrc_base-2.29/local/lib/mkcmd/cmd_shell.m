# $Id: cmd_shell.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
# After you include `cmd.h' you may include this tempate to provide	(ksb)
# a `shell' command to spawn an interactive subshell and wait for it.  Add
#	CMD_DEF_SHELL,
# to your table of commands.
#

require "cmd_shell.mc"
%i
static int cmd_shell();
#define CMD_DEF_SHELL	{ "shell", "spawn a subshell", cmd_shell, CMD_NULL, "[options]"}
%%
