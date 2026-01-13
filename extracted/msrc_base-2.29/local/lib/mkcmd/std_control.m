# most programs have a verbose/trace facility at PUCC			(ksb)
# $Id: std_control.m,v 8.12 2007/09/17 18:59:54 ksb Exp $
#
boolean 'v' {
	named "fVerbose"
	init '0'
	help "be verbose"
	key 2 init {
		"stderr"	# where to send verbose output
	}
}

boolean 'n' {
	named 'fExec'
	init '1'
	update '%n = !%i;%rvn = !%rvi;'
	help "do not execute commands, trace only"
}

# Set -n'd named to 1 to ignore any presumptuous fExec check -- ksb
# augment 'n' { user '%n = 1; /* force this always on */' }
# change -v in a similar way or change the key to force your own (FILE *)
