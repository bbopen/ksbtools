# $Id: cmd_exec.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
# After you include `cmd.h' you may include this tempate to provide	(ksb)
# a facility to include commands from the filesystem.
#
# Put the name of the command followed by a \0 followed by the full
# path to the shell command to be executed.  Put in the help, bind the
# action to cmd_fs_exec, and set the flags, and optional param notes.
#
# Note that cmd_parse.m
from '<stdio.h>'
from '<ctype.h>'

require "cmd_exec.mc"

%i
static int cmd_fs_exec();
#define CMD_DEF_PWD	{"pwd\0/bin/pwd", "print working directory", cmd_fs_exec, 0}
%%
