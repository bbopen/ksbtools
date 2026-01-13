# $Id: cmd_source.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
# After you include `cmd.h' you may include this tempate to provide	(ksb)
# a `source' command to source files of interpreted commands.  Add
#	CMD_DEF_SOURCE,
# to your table of commands.
#

require "cmd_source.mc"
%i
static int cmd_source();
#define CMD_DEF_SOURCE	{ "source", "interpret files of commands", cmd_source, 0, "[files]" }
%%
