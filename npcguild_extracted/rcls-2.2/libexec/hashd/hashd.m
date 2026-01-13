#!mkcmd
# $Id: hashd.m,v 2.9 2000/06/14 01:26:56 ksb Exp $
# See the README for details.
from '<sys/types.h>'
from '<sys/param.h>'
from '<unistd.h>'
from '<fcntl.h>'

require "std_help.m" "std_version.m"
require "hash.m"

basename "hashd" ""

%i
#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

static char rcsid[] =
	"$Id: hashd.m,v 2.9 2000/06/14 01:26:56 ksb Exp $";
%%

boolean 'i' {
	named "fInteractive"
	help "remove the network return code from the output"
}

fd ["O_RDWR, 0666"] named "fdLock" "pcLock" {
	param "counter"
	help "a file to hold the counter value (in binary)"
}
boolean 'P' {
	named "fPipe"
	user "%rin = 1;"
	help "pipe mode, produce hashes on stdout forever"
	integer 'p' {
		named "iPadSize"
		init "HASH_LENGTH+1"
		param "width"
		help "pad hash values to limit the number in the pipe"
	}
}
left "fdLock" {
	update 'if (fPipe) {Spin(%1n);}GetOne(%1n);'
}

%c
/* get a local hash value and spew it back				(ksb)
 * put an RFC 959 style return code on the data.
 */
static void
GetOne(fdLook)
int fdLook;
{
	register char *pcGot;
	auto char acHash[HASH_LENGTH+1];

	pcGot = NextHash(fdLook, acHash);
	if ((char *)0 == pcGot) {
		if (fInteractive) {
			fprintf(stderr, "%s: %s: state file corrupt?", progname, pcLock);
		} else {
			printf("451 hashd failed\n");
		}
		exit(1);
	}
	if (fInteractive) {
		printf("%s\n", pcGot);
	} else {
		printf("230 %s\n", pcGot);
	}
	exit(0);
}

/* just keep getting them (read by a program)				(ksb)
 */
static void
Spin(fdLook)
int fdLook;
{
	register char *pcGot, *pcMem;
	register int iWidth, iCc;

	if (iPadSize < HASH_LENGTH) {
		fprintf(stderr, "%s: hash values are longer than the pad param (%d < %d)\n", progname, iPadSize, HASH_LENGTH);
		exit(1);
	}
	pcMem = calloc(iPadSize, sizeof(char));
	if ((char *)0 == pcMem) {
		fprintf(stderr, "%s: calloc: no space\n", progname);
		exit(2);
	}
	do {
		pcGot = NextHash(fdLook, pcMem);
		if ((char *)0 == pcGot) {
			fprintf(stderr, "%s: %s: state file corrupt?", progname, pcLock);
#if USE_STRINGS
			bzero(pcMem, iPadSize);
#else
			memset(pcMem, '\000', iPadSize);
#endif
			pcGot = pcMem;
		}
		iWidth = 0;
		while (iWidth < iPadSize && -1 != (iCc = write(1, pcGot+iWidth, iPadSize-iWidth))) {
			iWidth += iCc;
		}
	} while (fPipe);
	exit(0);
}
