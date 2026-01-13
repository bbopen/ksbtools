# $Id: cmd_parse.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
# After you include `cmd.h' you may include this tempate to provide	(ksb)
# a facility to include commands from the filesystem.  The list of
# commands to install is read from a (FILE *).  The format of the
# file is:
#	[!] [@] path argv0 [params] -- helptext
# for example
#	/bin/rm rm [-rf] files -- remove files
# provides access to /bin/rm.
#
from '<stdio.h>'
from '<ctype.h>'

require "cmd_parse.mc"

%i
extern int cmd_parse();
%%
