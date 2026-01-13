#!mkcmd
# $Id: vinst.m,v 1.14 1999/06/01 17:27:17 ksb Exp $
# visual install, edit a file "in place" but use install to update it	(ksb)
#
# TODO: check link counts
#	check for "RCS/$target,v" as a sanity check
#	file locking, is it better to make them use flock(1)?
from '<sys/param.h>'
from '<sys/types.h>'
from '<sys/stat.h>'
from '<sys/file.h>'
from '<stdlib.h>'
from '<fcntl.h>'
from '<errno.h>'

require "std_help.m" "std_version.m"
require "util_editor.m" "util_tmp.m"
require "vinst.mh" "vinst.mc"

from '"main.h"'
from '"links.h"'

basename "vinst" ""
basename "vinstus" "-U"

augment action 'V' {
	user "Version();"
}
%i
extern int errno;
static char rcsid[] =
	"$Id: vinst.m,v 1.14 1999/06/01 17:27:17 ksb Exp $";
#define fInteractive 1	/* always */
%%


#boolean 'L' {
#	named "fLock"
#	init "0"
#	help "use a file lock while editing the file"
#}
boolean 'U' {
	named "fUserSupport"
	init "0"
	help "use installus rather than install to update the target"
}

char* named "pcTarget" {
	param "target"
	help "the file to update"
}

left "pcTarget" {
}

list {
	param "install-opts"
	help "options to be passed to install"
}

integer named "iExitCode" {
	init "0"
}
exit {
	update "exit(iExitCode);"
}

# just add the name of a program here to get BIN_name definde with the
# full path to the program.  Nifty mkcmd Trick.
key "vinst_paths" 8 init {
	"install"
	"installus"
	"vi"
	"diff"
	"instck"
}
# find me one of these can call it BIN_shell
key "vinst_shell" 1 init {
	"ksh" "sh" "bsh" "bash" "csh" "tcsh" "zsh" "rc"
}
