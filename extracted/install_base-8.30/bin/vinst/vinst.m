#!mkcmd
# $Id: vinst.m,v 1.20 2008/11/18 00:03:12 ksb Fix $
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
from '<unistd.h>'
from '<fcntl.h>'
from '<errno.h>'

require "std_help.m" "std_version.m"
require "util_editor.m" "util_tmp.m"
require "vinst.mh" "vinst.mc"

from '"vmachine.h"'
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
	"$Id: vinst.m,v 1.20 2008/11/18 00:03:12 ksb Fix $";
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
	"vi"
	"diff"
}
# These are also in override.m so we can force them for RPM builds
key "install_path" 8 init {
	"%D/install/p"
}
key "installus_path" 8 init {
	"%D/installus/p"
}
key "instck_path" 8 init {
	"%D/instck/p"
}
# find me one of these can call it BIN_shell
key "vinst_shell" 1 init {
	"sh" "ksh" "bash" "bsh" "csh" "tcsh" "zsh" "rc"
}
