# $Id: entomb.m,v 3.6 1997/09/10 14:42:56 ksb Exp $
# mkcmd for entomb
#
from '<sys/param.h>'
from '"entomb.h"'

require "std_help.m"

%i
#if HAS_DIRENT
#include <dirent.h>
#else
#include <sys/dir.h>
#endif
%%

action 'V' {
	update 'Version();'
	aborts 'exit(0);'
	from '"../libtomb/libtomb.h"'
	help "show version information"
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
/* output a version if for this program					(ksb)
 */
static void
Version()
{
	fprintf(stderr, "%s: %s\n", progname, "$Id: entomb.m,v 3.6 1997/09/10 14:42:56 ksb Exp $");
	fprintf(stderr, "%s: libtomb version %s\n", progname, libtomb_version);
}

/* Show ENTOMB if it's in our environment, otherwise show the default.
 */
static void
ShowEV()
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
