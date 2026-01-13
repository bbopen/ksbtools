# $Id: cmd_umask.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
# After you include `cmd.h' you may include this tempate to provide	(ksb)
# an `umask' command to change this process's umask.  Just add
#	CMD_DEF_UMASK
# to your table of commands.
#
# The support function "u_DecodeMask" takes a (char *) symbolic mode
# and a (mode_t) original mask to return the new mask.
#
# If you do NOT need "umask -S" support define NEED_SYMBOLIC_UMASK to 0.
#
from '<ctype.h>'

require "cmd_umask.mc"
%i
static int cmd_umask();
#define CMD_DEF_UMASK	{ "umask", "print/set file creation mask", cmd_umask, CMD_NULL, "[mask]"}

#if !defined(NEED_SYMBOLIC_UMASK)
#define NEED_SYMBOLIC_UMASK	1
#endif
%%
