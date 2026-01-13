# mkcmd script for untmp, which is a little unclean, anyway...
# $Compile(*): %b %o -mMkcmd %f && %b %o prog.c && mv prog %F
# $Mkcmd(*): ${mkcmd-mkcmd} std_control.m std_version.m std_help.m %f
#
from '<sys/types.h>'
from '<sys/stat.h>'
from '<errno.h>'
from '<unistd.h>'
from '<stdlib.h>'
from '<stdio.h>'
from '<pwd.h>'
from '<errno.h>'

from '"machine.h"'

boolean 'i' {
	named "fInteractive"
	init "0"
	help "confirm each action interactively"
}
boolean 'q' {
	named "fQuiet"
	init "0"
	help "be very quiet"
}
boolean 'r' {
	hidden named "fRecursive"
	init "0"
	help "recursively descend to directories owned by any target login"
}
accum[":"] 'T' {
	named "pcTempPath"
	init getenv "TMPPATH"
	help "path to search for crufty files"
}
after "DefValues();"
every {
	named "ScanFor"
	param "logins"
	help "users for which to remove files"
}
zero {
	named "ScanFor"
	help ""
}

%i
/* Copyright (c) 1979 Regents of the University of California */
/*
 * Remove stuff in /tmp owned by the invoker.
 * 
 * Modified 11/82 to accept the owning id as an argument (as documented!)
 * Mark Shoemaker, PUCC
 *
 * Converted to mkcmd by ksb (Kevin Braunsdorf), who else?  (ksb)
 */
static char rcsid[] =
	"$Id: untmp.m,v 1.9 2009/01/28 14:20:44 ksb Exp $";

#if HAVE_NDIR
#include <ndir.h>
#else
#if HAVE_DIRENT
#if USE_SYS_DIRENT_H
#include <sys/dirent.h>
#else
#include <dirent.h>
#endif
#else
#include <sys/dir.h>
#endif
#endif
%%

%c
/* if no temp path is set in the environment or the command line	(ksb)
 * make one up
 */
static void
DefValues()
{
	if ((char *)0 == pcTempPath) {
		pcTempPath = optaccum((char *)0, "/tmp:/var/tmp:/usr/tmp", ":");
	}
}

/* see if the user has chickend out on us				(ksb)
 */
static int
UserSaysNo(fp, piDef)
FILE *fp;
int *piDef;
{
	register int c;

	while (EOF != (c = getc(fp))) {
		if ('\n' == c)
			return *piDef;
		switch (c) {
		case 'y': case 'Y':  case '1':
			*piDef = 0;
			break;
		case 'n': case 'N': case 'q': case 'Q': case '0':
			*piDef = 1;
			break;
		case 'e': case 'E': case 's': case 'S': case 'o': case 'O':
		case 'u': case 'U': case 'i': case 'I': case 't': case 'T':
			break;
		default:
			break;
		}
	}
	return *piDef;
}

/* an old crufty program has put a little polish on			(ksb)
 */
static void
ScanFor(pcWho)
char *pcWho;
{
	auto struct stat stbuf;
	static char acQuit[] = ":";
	register int uid;
	register struct	DIRENT	*dirent;
	register char *pcTemp, *pcNext;
	register DIR *dirfile;
	register struct passwd *passent;

	if ((char *)0 == pcWho) {
		uid = getuid();
	} else if ( (passent = getpwnam(pcWho)) == 0 ) {
		fprintf(stderr, "%s: bad uid: %s\n", progname, pcWho);
		exit(1);
	} else {
		uid = passent->pw_uid;
	}
	/* loop though dirs and untmp them
	 */
	for (pcTemp = pcTempPath; (char *)0 != pcTemp && '\000' != *pcTemp; *pcNext++ = ':', pcTemp = pcNext) {
		if ((char *)0 == (pcNext = strchr(pcTemp, ':'))) {
			pcNext = acQuit;
		}
		*pcNext = '\000';

		if (chdir(pcTemp) < 0) {
			if (ENOENT != errno)
				fprintf(stderr, "%s: chdir: %s: %s\n", progname, pcTemp, strerror(errno));
			continue;
		}
		if ((dirfile = opendir(".")) == NULL) {
			fprintf(stderr, "%s: opendir: %s: %s\n", progname, pcTemp, strerror(errno));
			continue;
		}
		while ((dirent = readdir(dirfile)) != NULL) {
			if (dirent->d_ino == 0)
				continue;
			if (0 == strcmp(dirent->d_name, ".") || 0 == strcmp(dirent->d_name, ".."))
				continue;
			if (stat(dirent->d_name, &stbuf))
				continue;
			if (stbuf.st_uid != uid)
				continue;

#if defined(S_IFLNK)
			/* symbolic links are close enough -- ksb
			 */
			if ((stbuf.st_mode & S_IFMT) == S_IFLNK)
				/* OK */;
			else
#endif
			if ((stbuf.st_mode & S_IFMT) != S_IFREG)
				continue;
			if (fInteractive) {
				static int fLast = 0;

				printf("%s: rm -f %s [%s]? ", progname, dirent->d_name, fLast ? "ny" : "yn");
				fflush(stdout);
				if (UserSaysNo(stdin, & fLast))
					continue;
			}
			if (fExec && 0 != unlink(dirent->d_name)) {
				continue;
			}

			if (fVerbose) {
				printf("%s: rm -f %s\n", progname, dirent->d_name);
			} else if (!fQuiet) {
				printf("%s\n", dirent->d_name);
			}
		}
		(void)closedir(dirfile);
	}
}
