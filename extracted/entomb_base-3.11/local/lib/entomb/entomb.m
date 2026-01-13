# $Id: entomb.m,v 3.10 2008/11/17 18:54:38 ksb Exp $
# mkcmd for entomb
#
from '<sys/param.h>'
from '"machine.h"'
from '"../libtomb/libtomb.h"'
from '"entomb.h"'

require "std_help.m" "std_version.m"

%i
static char rcsid[] =
	"$Id: entomb.m,v 3.10 2008/11/17 18:54:38 ksb Exp $";

#if HAVE_DIRENT
#include <dirent.h>
#else
#include <sys/dir.h>
#endif
%%

augment action 'V' {
	user 'printf("%%s: libtomb version %%s\\n", %b, libtomb_version);'
}

int variable "iHow" {
	param "how"
	help "tell how to entomb the file (rename|copy)"
	init "RENAME"
}

after {
	named "EntombInit"
	from '"../libtomb/libtomb.h"'
	update "%n();"
}

excludes "cC"
boolean 'c' {
	help "copy file to tomb"
	update 'iHow = COPY;'
}

boolean 'C' {
	help "copy file to tomb, unlink original"
	update 'iHow = COPY_UNLINK;'
}

action 'E' {
	named 'ShowEV'
	aborts 'exit(0);'
	help 'explain the value of ENTOMB'
}

integer variable "iLastErr" {
	init "0"
}

every {
	named "Entomb"
	param "file"
	from '"../libtomb/libtomb.h"'
	update "iLastErr |= %n(iHow, %a);"
	help "file to entomb"
}

exit {
	update "/* tell the called we did it */"
	abort "exit(iLastErr);"
}

%c

/* Show ENTOMB if it's in our environment, otherwise show the default.
 */
static void
ShowEV(int iOpt)		/*ARGSUSED*/
{
	extern char *getenv();
	char *pcEnv;

	if ((char *)0 == (pcEnv = getenv("ENTOMB"))) {
		fprintf(stderr, "%s: ENTOMB=%s\n", progname, DEFAULT_EV);
		return;
	}
	fprintf(stderr, "%s: ENTOMB=%s\n", progname, pcEnv);
}
%%
