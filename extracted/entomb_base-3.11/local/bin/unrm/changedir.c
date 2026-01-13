/* $Id: changedir.c,v 3.7 2008/11/17 18:59:05 ksb Exp $
 * do the changedir command (each command has a file)
 */
#include <stdio.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#include "machine.h"
#include "libtomb.h"
#include "main.h"
#include "lists.h"
#include "init.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#else
extern char *getenv();
#endif

extern int errno;
#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me)	(sys_errlist[Me])
#endif


/* for interactive mode, the user can use the "cd" command		(mjb)
 * to switch the directory that files get unremoved to.  What we do
 * is, we switch the real and effective user-id's, call chdir, and
 * switch them back.
 */
int
cm_ChDir(argc, argv)
int argc;
char **argv;
{
	auto char sb[(MAXPATHLEN|3)+1];
	int errs = 0;
	register char *pcDir;

	switch (argc) {
	case 1:
		if ((char *)0 == (pcDir = getenv("HOME"))) {
			(void)fprintf(stderr, "%s: cd: cannot find $HOME\n", progname);
			return 1;
		}
		break;
	case 2:
		pcDir = argv[1];
		break;
	default:
		(void)fprintf(stderr, "%s: cd: usage [dir]\n", progname);
		return 1;
	}

	user();
	if (-1 == chdir(pcDir)) {
		(void)fprintf(stderr, "%s: chdir: %s: %s\n", progname, pcDir, strerror(errno));
		++errs;
	}
	if (Verbose) {
		if((char *)0 == 
#if HAVE_GETCWD
		    getcwd(sb, sizeof(sb))
#else
		    getwd(sb)
#endif
		) {
			(void)fprintf(stderr, "%s: get pwd: %s\n", progname, sb);
			exit(2);
		}
		if (0 == errs)
			(void)fprintf(stdout, "Directory changed to %s.\n", sb);
		else 
			(void)fprintf(stdout, "Directory unchanged.\n");
	}
	charon();
	return 0 != errs;
}
