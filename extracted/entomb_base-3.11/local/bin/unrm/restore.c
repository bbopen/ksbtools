/* $Id: restore.c,v 3.16 2008/11/17 18:59:05 ksb Exp $
 * do the restore command (each command has a file)
 */
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <stdlib.h>

#include "machine.h"
#include "libtomb.h"
#include "main.h"
#include "restore.h"
#include "lists.h"
#include "init.h"

#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me)	(sys_errlist[Me])
#endif

#if USE_FCNTL_H
#include <fcntl.h>
#endif

/* stat the file to see if it exists					(ksb)
 */
static int
UserSees(pc)
char *pc;
{
	auto struct stat stThis;
	register int i;

	user();
	i = STAT_CALL(pc, & stThis);
	charon();

	if (-1 == i) {
		return 0;
	}
	return 1;
}

/* try to entomb the file, if we can't ask about it			(ksb)
 */
int
ForceTomb(pcFile)
char *pcFile;
{
	register int iEntombPid, iPid, i;
	extern char **environ;
	auto char *apcArgv[4];
	static char acPathEntomb[] = ENTOMB;
#if HAS_UNIONWAIT
	auto union wait status;
#else
	auto int status;
#endif

	switch (iEntombPid = fork()) {
	case -1:
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		return 1;
	case 0:
		apcArgv[0] = "entomb";
		apcArgv[1] = "-C";
		apcArgv[2] = pcFile;
		apcArgv[3] = (char *)0;
		execve(acPathEntomb, apcArgv, environ);
		fprintf(stderr, "%s: execve: %s: %s\n", progname, acPathEntomb, strerror(errno));
		exit(NO_PATH);
	default:
		break;
	}
	while (-1 != (iPid = wait(&status))) {
		if (iEntombPid != iPid) {
			continue;
		}
#if HAS_UNIONWAIT
		i = status.w_retcode;
#else
		i = 0xff&(status >> 8);
#endif
		break;
	}
	switch (i) {
	case 0:
		return 0;
	case WE_CROAK:
		printf("a system error occured");
		break;
	case NOT_ENTOMBED:
		printf("that file could not be entombed");
		break;
	case NO_TOMB:
		printf("there is not tomb on the filesystem");
		break;
	case NO_PATH:
		printf("my path to the entomb binary is wrong");
		break;
	default:
		printf("something really terrible happened");
		break;
	}
	printf(" -- aborting this restore.\n");
	return 1;
}

/* restore an entombed file, and delete it from the tomb.		(ksb)
 */
int
Restore(pLE)
LE *pLE;
{
	register int fdIn, fdOut;
	register int iBytes;		/* num of bytes from read */
	register char *pcEFile;
	auto struct stat statBuf; /* to get the modes			*/
	auto char acAnswer[5];	/* enough room for 'yes\n'		*/
	auto char acBuf[1024];
	auto char acReal[1024];	/* the file's real name			*/

	pcEFile = pLE->name;
	if ((char *)0 == real_name(acReal, pLE)) {
		return 1;
	}

	(void)fprintf(stdout, "%s: restoring %s\n", progname, pcEFile);

	/* yes, entomb it, no, quit!, help me?
	 */
	while (!Force && UserSees(pcEFile)) {
		register char *pcAns;

		(void)fprintf(stdout, "%s: %s exists: overwrite? [eynqh] ", progname, pcEFile);
		(void)fflush(stdout);
		(void)fgets(acAnswer, sizeof(acAnswer), stdin);
		clearerr(stdin);
		pcAns = acAnswer;
		while (isspace(*pcAns) && '\n' != *pcAns)
			++pcAns;
		switch (*pcAns) {
		case 'y':
		case 'Y':
			break;
		case 'n':
		case 'N':
			UnMark(pLE);
			return 0;
		case 'q':
		case 'Q':
			exit(0);
		case '\n':
		case 'e':
		case 'E':
			/* entomb the monster with `entomb -C' */
			if (0 != ForceTomb(pcEFile)) {
				UnMark(pLE);
				/* XXX say it failed and continue? -- ksb */
				return 0;
			}
			break;
		case 'h':
		case 'H':
		case '?':
			printf("e  entomb the current file, then replace it\n");
			printf("h  print this help message\n");
			printf("n  don't change the file\n");
			printf("q  quit this program\n");
			printf("y  yes, overwrite the file with the entombed one\n");
			continue;
		}
		if (-1 == unlink(pcEFile) && ENOENT != errno) {
			fprintf(stderr, "%s: unlink: %s: %s\n", progname, pcEFile, strerror(errno));
			UnMark(pLE);
			return 1;
		}
	}

	/* open the file to copy out of the tomb
	 */
	if (-1 == (fdIn = open(acReal, O_RDONLY, 0))) {
		(void)fprintf(stderr, "%s: open: %s: %s\n", progname, acReal, strerror(errno));
		UnMark(pLE);
		return 1;
	}
	if (-1 == fstat(fdIn, &statBuf)) {
		statBuf.st_mode = S_IFREG|0644;
	}


	/* open the output file as joe user
	 * copy --  and unlink 
	 * end as charon in any case
	 */
	user();
	if (-1 != (fdOut = open(pcEFile, O_TRUNC|O_CREAT|O_WRONLY, statBuf.st_mode))) {
		while (0 < (iBytes = read(fdIn, acBuf, sizeof(acBuf)))) {
			int iStatus;
			if (iBytes != (iStatus = write(fdOut, acBuf, iBytes))) {
				if (-1 == iStatus) {
					(void)fprintf(stderr, "%s: write: %s: %s\n", progname, pcEFile, strerror(errno));
					exit(1);
				}
			}
		}
		(void)close(fdOut);
		charon();
		if (-1 == unlink(acReal)) {
			UnMark(pLE);
		}
	} else {
		(void)fprintf(stderr, "%s: open: %s: %s\n", progname, pcEFile, strerror(errno));
		UnMark(pLE);
		charon();
	}
	(void)close(fdIn);
	return 0;
}
