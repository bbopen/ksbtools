# mkcmd source for "unrm"
# $Compile: mkcmd std_help.m std_targets.m %f
# Matthew Bradburn, PUCC UNIX Group
# $Id: preend.m,v 3.3 2000/07/31 15:14:39 ksb Exp $

require "std_help.m"

init "Init(argv);"

boolean 'd' {
	named "Debug"
	initializer "0"
	help "turn on debugging output"
}
boolean 'n' {
	named "fExec"
	initializer "1"
	help "do not really take any actions"
}
boolean '1' {
	named "fOnce"
	initializer "0"
	help "just run one preen run, synchronously"
}
boolean 'p' {
	named "fSingle"
	initializer "1"
	help "run one process per file system"
}
boolean 'R' {
	named "fEmpty"
	initializer "0"
	help "remove empty crypts (dangerous)"
}
integer "s" {
	named "sleeptime"
	initializer "5"
	param "minutes"
	help "sleep time between filesystem checks (default %i)"
}
action 'V' {
	update 'Version();'
	aborts 'exit(0);'
	help "show version information"
}
after {
	update "After();"
}
list {
	named "PreenList"
	update "exit(%n(%#, %@));"
	param "tombs"
	help "limit preen to tombs, if any are given"
}
exit {
	update ""
	aborts "/*NOTREACHED*/"
}
