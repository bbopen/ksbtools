# command line options for purge(8L)
# $Compile: ${mkcmd-mkcmd} -n main std_help.m %f
# $Id: purge.m,v 8.1 1997/03/11 19:36:43 ksb Exp $


after {
	named "Version"
	update "InitAll();if (%rVn) {exit(%n());}"
}

from '"machine.h"'
from '"configure.h"'
from '"install.h"'

every {
	from '"purge.h"'
	named "Which"
	parameter "dirs"
	help "dirs to purge"
}

# this puts the []'s on dirs for us,
# of course we could 'Scan("/");' here if we were not so conservative
zero {
	update "exit(0);"
}

exit {
	named 'Done'
	update '%n();'
}

# we don't do this because it is never what we want,
# we really want the last stable version --
# we don't know how to find that.
#boolean '1' {
#	named 'f1Copy'
#	init '0'
#	help "always keep one copy as a backup"
#}

boolean 'A' {
	named "fAnyOwner"
	help "purge for all users"
}

function 'u' {
	parameter "user"
	named "AddHer"
	update "(void)%n(%a);"
	help "OLD directories may be owned by this user"
}

boolean 'v' {
	named "fVerbose"
	help "be verbose"
}

boolean 'n' {
	named "fExec"
	init '1'
	update "%n = !%i;%rvn = !%rvi;"
	help "do not really execute commands"
}

integer 'd' {
	verify named "iDays"
	parameter "days"
	init '14'
	help "days to keep backup files (default %i)"
}

boolean 'V' {
	named "fVersion"
	init "0"
	help "show version information"
}

boolean 'S' {
	named "fSuperUser"
	init "0"
	help "run as if we were the superuser"
}
