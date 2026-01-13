# mkcmd script for untmp, which is a little unclean, anyway...
# $Compile(*): %b %o -mMkcmd %f && %b %o prog.c && mv prog %F
# $Mkcmd(*): ${mkcmd-mkcmd} std_control.m std_version.m std_help.m %f
#
from '<sys/types.h>'
from '<sys/stat.h>'
from '<errno.h>'
from '<stdio.h>'
from '<pwd.h>'

from '"machine.h"'

boolean 'i' {
	hidden named "fInteractive"
	init "0"
	help "confirm each action interactively"
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
after {
	named "DefValues"
}
every {
	named "ScanFor"
	param "logins"
	help "users for which to remove files"
}
zero {
	named "ScanFor"
	help ""
}

%%
/* Copyright (c) 1979 Regents of the University of California */
/*
 * Remove stuff in /tmp owned by the invoker.
 * 
 * Modified 11/82 to accept the owning id as an argument (as documented!)
 * Mark Shoemaker, PUCC
 *
 * Converted to mkcmd by ksb (Kevin Braunsdorf), who else?  (ksb)
 */
#ifdef pdp11
#include <ndir.h>
#define DIRENT	direct
#else
#if SUN5
#include <dirent.h>
#define DIRENT	dirent
#else
#include <sys/dir.h>
#define DIRENT	direct
#endif
#endif

extern int errno;

static char rcsid[] =
	"$Id: untmp.m,v 1.6 1996/08/09 17:25:24 kb207252 Exp $";

extern struct passwd *getpwnam();


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

/* an old crufty program has put a little polish on			(ksb)
 */
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

			/* ZZZ symbolic links?
			 */
			if ((stbuf.st_mode & S_IFMT) != S_IFREG)
				continue;
			if (fExec && 0 != unlink(dirent->d_name))
				continue;

			if (fVerbose)
				printf("%s: rm -f %s\n", progname, dirent->d_name);
			else
				printf("%s\n", dirent->d_name);
		}
		(void)closedir(dirfile);
	}
}
