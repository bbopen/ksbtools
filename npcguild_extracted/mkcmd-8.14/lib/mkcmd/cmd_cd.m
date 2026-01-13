# $Id: cmd_cd.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
# After you include `cmd.h' you may include this tempate to provide	(ksb)
# an `cd' command to change this process's current working directory.  Just add
#	CMD_DEF_CD,
# or
#	CMD_DEF_CHDIR,
# to your table of commands.
#
from '<sys/param.h>'

require "util_glob.m" "util_home.m" "cmd_cd.mc"

%i
static int cmd_cd();
#define CMD_DEF_CD	{ "cd", "change directory", cmd_cd, CMD_NULL, "[directory]"}
#define CMD_DEF_CHDIR	{ "chdir", "change directory", cmd_cd, CMD_NULL, "[directory]"}
%%
