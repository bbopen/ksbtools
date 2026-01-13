# most programs have a verbose/trace facility at PUCC			(ksb)
# $Id: std_control.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
#
boolean 'v' {
	named "fVerbose"
	init '0'
	help "be verbose"
}

boolean 'n' {
	named 'fExec'
	init '1'
	update '%n = !%i;%rvn = !%rvi;'
	help "do not execute commands, trace only"
}
