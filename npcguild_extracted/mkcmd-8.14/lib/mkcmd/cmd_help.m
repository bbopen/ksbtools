# $Id: cmd_help.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
# This provides a nice on-line help for your command interpreter.
# After you add `cmd.m' you mae add this template and the line		(ksb)
#	CMD_DEF_HELP,
# to your table of command.
# You may also add
#	CMD_DEF_COMMANDS,
# to get a `terse list of commands' command
#
require "cmd_help.mc"

%i
static int cmd_help();
#define CMD_DEF_HELP	{ "help", "output a help message", cmd_help, 0, "[commands]" }
static int cmd_commands();
#define CMD_DEF_COMMANDS { "commands", "output a terse list of commands", cmd_commands, 0 }
%%

# API key for help banner messages
key "help_banner" 1 init {
}
