# mkcmd parser for ptyd						(ksb)
# $Compile: mkcmd -o std_help.m std_version.m %f
from '"ptyd.h"'

require "std_help.m" "std_version.m"

basename "ptyd" ""

%i
static char rcsid[] =
	"$Id: ptyd.m,v 1.7 1997/11/29 17:58:50 ksb Exp $";
%%

boolean 'n' {
	named "fExec"
	init "1"
	help "trace actions only"
}

char* 'p' {
	named "pcProto"
	param "pty"
	init '"/dev/ptyp0"'
	help "a sample pty for us to stat to get the device numbers"
}

boolean 'd' {
	named "fDebug"
	init '0'
	help "start in debug mode"
}

boolean 'i' {
	named "fInetd"
	init '0'
	help "enable starting by inetd"
}

action 'c' {
	update "Clean();exit(0);"
	help "ask the daemon to reset lots of ptys"
}

action 's' {
	update "Status();exit(0);"
	help "ask the daemon for a status output"
}

action 'k' {
	update "KillMe();exit(0);"
	help "kill any running daemon"
}

pointer 'm' {
	named "pcDefMach"
	init '"localhost"'
	param "machine"
	help "default machine people are on from"
}

pointer 'f' {
	named "pcPort"
	init "PTYD_PORT"
	param "port"
	help "give the filename for ptyd to listen on"
}

int named "iMaxForks" {
	param "maxservers"
	init '1'
	help "maximum number of servers to fork"
}

left [ "iMaxForks" ] {
}

exit {
	named "Server" from '"ptyd.h"'
	update "%n();"
}
