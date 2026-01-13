#!mkcmd
# User interface definition for project -- ksb

require "std_help.m" "std_version.m"

# force mkcmd to find system paths
require "path.m"

basename "project" ""
routine "options"

%i
static char rcsid[] =
	"$Id: project.m,v 4.19 1997/02/20 19:57:07 ksb Exp $";
%%

from '"project.h"'

# options for everyone
char* 'c' {
	named "pcCourse"
	param "course"
	help "select which course to administer"
}
boolean 'n' {
	named "fExec"
	init "1"
	help "just print what would be done"
}
boolean 'v' {
	named "fVerbose"
	help "output shell commands and be verbose"
}

# only one of the commands, please
exclude "deilopqr"

char* 'G' {
	named "pcSum"
	param "cmd"
	update "%n = %a;What = GRADE;"
	help "execute command for each division"
	exclude "deilopqrG"
}
char* 'g' {
	named "pcCmd"
	param "cmd"
	update "%n = %a;What = GRADE;"
	help "execute command for all submitted files"
	exclude "deilopqrg"
}

action 'd' {
	update "What = DISABLE;"
	help "allow no submissions for a while"
	exclude "gG"
}
action 'e' {
	update "What = ENABLE;"
	help "allow submissions and change default project name"
	exclude "gG"
}
action 'l' {
	update "What = LATE;"
	help "allow submissions without changing default project"
	exclude "gG"
}
action 'i' {
	update "What = INITIALIZE;"
	help "initialize an account for submissions"
	exclude "gG"
}
action 'o' {
	update "What = OUTPUT;"
	help "display a status line"
	exclude "gG"
}
action 'p' {
	update "What = PACKSD;"
	help "tar and compress a disabled project"
	exclude "gG"
}
action 'q' {
	update "What = QUIT;"
	help "quit using this account for submissions"
	exclude "gG"
}
action 'r' {
	update "What = REMOVE;"
	help "delete a project"
	exclude "gG"
}

# and a project to do this to
char* variable "pcProject" {
	param "name"
	help "the name of the project to alter"
}
left [ "pcProject" ] {
}
