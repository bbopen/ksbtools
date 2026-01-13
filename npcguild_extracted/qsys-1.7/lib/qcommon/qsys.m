# common options to all qsys command					(ksb)
# $Id: qsys.m,v 1.2 1994/02/10 19:12:06 ksb Exp $
#
boolean 'n' {
	named "fExec"
	init "1"
	user "%rvT"
	help "just output a trace of commands run"
}
boolean 'v' {
	named "fVerbose"
	help "be verbose"
}

char* 'd' {
	named "pcQDir"
	param "dir"
	init "Q_DIR"
	user "if ('/' != %n[0]) {static char ac_M[1024];(void)sprintf(ac_M, \"%%s/%%s\", Q_SPOOL, %n);%n = ac_M;}"
	help "queue to manipulate"
}
