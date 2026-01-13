/* $Id: explode.c,v 6.0 2000/04/10 20:18:54 ksb Exp $
 * This program burstes M.I.C.E. archieves into common C code.		(ksb)
 *
 * Kevin S Braunsdorf
 * ksb@sa.fedex.com
 */
#include <sys/param.h>
#include <sys/file.h>
#include <signal.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <pwd.h>

#include "machine.h"
#include "explode.h"
#include "main.h"

extern struct passwd *getpwnam();

#if HAVE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if HAVE_STDLIB
#include <stdlib.h>
#else
extern char *malloc(), *realloc();
#endif

/* we have to die because we are out of core				(ksb)
 */
static void
OutOfMem()
{
	static char acFailMem[] = ": out of memory\n";

	write(2, progname, strlen(progname));
	write(2, acFailMem, sizeof(acFailMem)-1);
	exit(ENOMEM);
}

static char
	acNull[]= "/dev/null",	/* the bit bucket			*/
	acTty[]= "/dev/tty",	/* the users tty			*/
	acAppend[]= "Append",
	acHead[]= "Header",
	acExp[] = "Explode",
	acRem[] = "Remove",
	acVersion[] = "Version",
	acIns[] = "Insert",
	acShell[] = "Shell",
	acMsg[] = "Message";
#define MAXLINE		2048	/* max C code line ANSI sez 509		*/

static char
	*pcTemp,		/* the real temp file name		*/
	acTemp[] =		/* file name template for header	*/
		"/tmp/explXXXXXX",
	acStdin[] =		/* internal name for "-" on cmd line	*/
		"stdin";

/* search a path (colon separated) for a directory that has the file	(ksb)
 * fall back on ./file always first, and full paths are not scanned
 */
static FILE *
SearchOpen(pcPath, pcFile, pcHow)
char *pcPath, *pcFile, *pcHow;
{
	register char *pcNext;
	static char acColon[] = ":";
	register FILE *fpFound;
	auto char acTry[MAXPATHLEN+1];

	if (acStdin == pcFile) {
		if (fPrint) {
			printf("-\n");
		}
		return stdin;
	}
	if ((FILE *)0 != (fpFound = fopen(pcFile, pcHow)) || '/' == pcFile[0]) {
		if (fPrint && (FILE *)0 != fpFound) {
			printf("%s\n", pcFile);
		}
		return fpFound;
	}

	for (; *pcPath != '\000'; (*pcNext = ':'), pcPath = pcNext+1) {
		pcNext = strchr(pcPath, ':');
		if ((char *)0 == pcNext)
			pcNext = acColon;
		*pcNext = '\000';

		if ('~' == pcPath[0]) {
			register char *pcSlash;
			register struct passwd *pwd;
			static char acSlash[] = "/";

			if ((char *)0 == (pcSlash = strchr(pcPath+1, '/')))
				pcSlash = acSlash;
			*pcSlash = '\000';
			pwd = ('\000' == pcPath[1]) ? getpwuid(getuid()) : getpwnam(pcPath+1);
			*pcSlash = '/';
			if ((struct passwd *)0 == pwd) {
				continue;
			}
			sprintf(acTry, "%s/%s/%s", pwd->pw_dir, pcSlash+1, pcFile);
		} else {
			sprintf(acTry, "%s/%s", pcPath, pcFile);
		}
		if (fVerbose) {
			fprintf(stderr, "%s: search for %s as %s\n", progname, pcFile, acTry);
		}
		if ((FILE *)0 != (fpFound = fopen(acTry, pcHow))) {
			*pcNext = ':';
			if (fPrint) {
				printf("%s\n", acTry);
			}
			return fpFound;
		}
	}
	return (FILE *)0;
}

/* make sure global flages are consistant				(ksb)
 * and build temp file name
 */
void
SetGlobals()
{
	extern char *mktemp();

	if (!fActive)
		fVerbose = 1;
	pcTemp = mktemp(acTemp);
}


/* zap the temp file							(ksb)
 */
void
RemoveTemp()
{
	unlink(pcTemp);
}

/* tell if we need this file						(ksb)
 * We need all files if none are asked for, we need only those
 * in the list is any are asked for.
 */
static int
Need(pc)
char *pc;
{
	register char *pcCur, *pcNext;
	static char acComma[] = ",";

	if ((char *)0 == (pcCur = pcUnpack)) {
		return 1;
	}
	for (; *pcCur != '\000'; (*pcNext = ','), pcCur = pcNext+1) {
		pcNext = strchr(pcCur, ',');
		if ((char *)0 == pcNext)
			pcNext = acComma;
		*pcNext = '\000';

		if (0 == strcmp(pc, pcCur)) {
			*pcNext = ',';
			return 1;
		}
	}
	return 0;
}

/* qwery the user WRT this file, should we alter it			(ksb)
 */
static int
Asked(pcFile)
char *pcFile;
{
	static char *pcTold = (char *)0;
	auto char acAns[32];
	register char *pcCur, *pcNext;
	static char acComma[] = ",";

	if (!fAsk) {
		return 0;
	}
	/* is this in the list we already asked about?
	 */
	if ((char *)0 != (pcCur = pcTold)) {
		for (pcCur = pcTold; *pcCur != '\000'; (*pcNext = ','), pcCur = pcNext+1) {
			pcNext = strchr(pcCur, ',');
			if ((char *)0 == pcNext)
				pcNext = acComma;
			*pcNext = '\000';
			if (0 == strcmp(pcFile, pcCur+1)) {
				*pcNext = ',';
				return 'y' == *pcCur;
			}
		}
	}
	fprintf(stdout, "%s: modify %s? [ny] ", progname, pcFile);
	fflush(stdout);
	if ((char *)0 == fgets(acAns, sizeof(acAns), stdin)) {
		fprintf(stdout, "no\n");
		acAns[0] = 'n';
	}
	for (pcCur = acAns; isspace(*pcCur); ++pcCur)
		/* nothing */;

	acAns[0] = ('Y' == *pcCur || 'y' == *pcCur) ? 'y' : 'n';

	if ((char *)0 == pcTold) {
		pcTold = pcCur = malloc(1+strlen(pcFile)+1);
		if ((char *)0 == pcTold) {
			OutOfMem();
		}
	} else {
		register int i;
		i = strlen(pcTold);
		pcTold = realloc(pcTold, i+1+1+strlen(pcFile)+1);
		if ((char *)0 == pcTold) {
			OutOfMem();
		}
		pcCur = pcTold + i;
		*pcCur++ = ',';
	}
	*pcCur++ = acAns[0];
	(void)strcpy(pcCur, pcFile);
	return 'y' == acAns[0];
}

/* does all the work of unpacking the files from the MICE archive	(ksb)
 */
void
UnPack(pcFile, pcBase)
char *pcFile, *pcBase;
{
	static char acPath[MAXPATHLEN+1], acWMode[] = "w", acAMode[] = "a";
	auto char acLine[MAXLINE+1];
	auto char acBase[255];
	register FILE *fpIn, *fpOut;
	register char *pcMode, *pc;
	register FILE *fpHead;
	register int fNewHeader;

	if (NULL == pcFile || ('-' == pcFile[0] && '\000' == pcFile[1])) {
		pcFile = acStdin;
		if ((char *)0 == pcBase) {
			pcBase = acStdin;
		}
	} else if (NULL == pcBase) {
		pc = strrchr(pcFile, '/');
		if (NULL == pc)
			pc = pcFile;
		else
			++pc;
		(void)strcpy(acBase, pc);
		pc = strrchr(acBase, '.');
		if (NULL != pc)
			*pc = '\000';
		pcBase = acBase;
	}

	SetDelExt(pcFile);

	if (fVerbose) {
		if (fTable)
			fprintf(stdout, "%s: contents of ", progname);
		else
			fprintf(stdout, "%s: unpack ", progname);
		if ((char *)0 != pcUnpack)
			fprintf(stdout, "only %s in ", pcUnpack);
		fprintf(stdout, "%s to %s*%s\n", acStdin == pcFile ? "<stdin>" : pcFile, pcBase, pcExt);
	}

	if (NULL == (fpIn = SearchOpen(pcIncl, pcFile, "r"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, pcFile, strerror(errno));
		exit(1);
	}
	if (fPrint) {
		return;
	}

	fpOut = (FILE *)0;
	if (fWhole) {
		if ('-' != pcBase[0] || '\000' != pcBase[1]) {
			sprintf(acPath, "%s%s", pcBase, pcExt);
			if (0 == fOverwrite && 0 == access(acPath, F_OK) && 0 == Asked(acPath)) {
				fprintf(stderr, "%s: won't overwrite %s\n", progname, acPath);
				exit(9);
			}
			if (fVerbose) {
				fprintf(stdout, "%s: found %s\n", progname, acPath);
			}
			if (! fActive) {
				goto get_out;
			}
			if (NULL == (fpOut = fopen(acPath, acWMode))) {
				fprintf(stderr, "%s: fopen: %s: %s\n", progname, acPath, strerror(errno));
				exit(2);
			}
		} else if (! fActive) {
			goto get_out;
		} else {
			fpOut = stdout;
		}
		while (NULL != fgets(acLine, sizeof(acLine), fpIn)) {
			fputs(acLine, fpOut);
		}
		goto get_out;
	}

	if (NULL == (fpHead = fopen(pcTemp, "w"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, pcTemp, strerror(errno));
		exit(1);
	}
#ifdef DEBUG
	fprintf(fpHead, "%s no header given */\n", pcDel);
#endif /* DEBUG */
	fclose(fpHead);

	if ((FILE *)0 == (fpOut = fopen(acNull, "w"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, acNull, strerror(errno));
		exit(1);
	}
	fNewHeader = 0;
	while (NULL != fgets(acLine, sizeof(acLine), fpIn)) {
		if (0 != strncmp(pcDel, acLine, iDelLen)) {
			fputs(acLine, fpOut);
			continue;
		}
		pc = & acLine[iDelLen];
		while (isspace(*pc))
			++pc;
		pcMode = acWMode;
		if (fTable) {
			register char *pcFind;
			static char acCurVer[2048];

			if (0 == strncmp(pc, acVersion, sizeof(acVersion)-1)) {
				pc += sizeof(acVersion)-1;
				while (isspace(*pc))
					++pc;
				pcFind = acCurVer;
				while (*pc != cClose)
					*pcFind++ = *pc++;
				*pcFind = '\000';
				continue;
			} else if (0 == strncmp(pc, acExp, sizeof(acExp)-1)) {
				pc += sizeof(acExp);
			} else if (0 == strncmp(pc, acHead, sizeof(acHead)-1)) {
				pc += sizeof(acHead);
			} else {
				continue;
			}
			while (isspace(*pc))
				++pc;
			pcFind = pc;
			while (!isspace(*pc) && *pc != cClose)
				++pc;
			*pc = '\000';
			if ('\000' == *pcFind)
				pcFind = "*";
			else if ((char *)0 != pcUnpack && ! Need(pcFind))
				continue;
			printf("%s\t%s%s%s", pcFile, pcBase, pcFind, pcExt);
			if ('\000' != acCurVer[0])
				printf("\t%s", acCurVer);
			printf("\n");
			continue;
		}
		if (0 == strncmp(pc, acExp, sizeof(acExp)-1) ||
		   (pcMode = acAMode,
		   0 == strncmp(pc, acAppend, sizeof(acAppend)-1))) {
			register char *pcFind;

			pc += pcMode == acExp ? sizeof(acExp) : sizeof(acAppend);
			while (isspace(*pc))
				++pc;
			pcFind = pc;
			while (!isspace(*pc) && *pc != cClose)
				++pc;
			*pc = '\000';
			if ((char *)0 != pcUnpack && ! Need(pcFind)) {
				goto dev_null;
			}
			if (stdout != fpOut) {
				fclose(fpOut);
			}
			if ('-' != pcBase[0] || '\000' != pcBase[1]) {
				sprintf(acPath, "%s%s%s", pcBase, pcFind, pcExt);
				if (0 == fOverwrite && acWMode == pcMode &&
				    0 == access(acPath, F_OK) &&
				    0 == Asked(acPath)) {
					fprintf(stderr, "%s: won't overwrite %s\n", progname, acPath);
					exit(9);
				}
				if (fVerbose) {
					fprintf(stdout, "%s: %s %s\n", progname, acWMode == pcMode ? "create" : "append", acPath);
				}
				if (! fActive) {
					continue;
				}
				if (NULL == (fpOut = fopen(acPath, pcMode))) {
					fprintf(stderr, "%s: fopen: %s: %s\n", progname, acPath, strerror(errno));
					exit(2);
				}
			} else if (! fActive) {
				continue;
			} else {
				fpOut = stdout;
				if (! fNewHeader) {
					continue;
				}
			}
			if (acWMode != pcMode) {
				continue;
			}
			if (NULL == (fpHead = fopen(pcTemp, "r"))) {
				fprintf(stderr, "%s: fopen: %s: %s\n", progname, pcTemp, strerror(errno));
				exit(3);
			}
			while (NULL != fgets(acLine, sizeof(acLine), fpHead)) {
				fputs(acLine, fpOut);
			}
			fclose(fpHead);
			fNewHeader = 0;
		} else if (0 == strncmp(pc, acIns, sizeof(acIns)-1)) {
			register int iStop;

			if (fVerbose)
				fprintf(stdout, "%s: insert text\n", progname);
			pc += sizeof(acIns)-1;
			while (isspace(*pc))
				++pc;
			iStop = *pc;
			while (iStop != *++pc)
				putc(*pc, fpOut);
			putc('\n', fpOut);
		} else if (0 == strncmp(pc, acShell, sizeof(acShell)-1)) {
			register int iStop;
			register char *pcCmd;
			register FILE *fpShell;

			pc += sizeof(acShell)-1;
			while (isspace(*pc))
				++pc;
			iStop = *pc;
			pcCmd = pc+1;
			while (iStop != *++pc)
				;
			*pc = '\000';
			if (fVerbose)
				fprintf(stdout, "%s: shell `%s'\n", progname, pcCmd);
			if ((FILE *)0 == (fpShell = popen(pcCmd, "r"))) {
				fprintf(stdout, "%s: shell fails\n", progname);
				exit(1);
			}
			while (EOF != (iStop = getc(fpShell))) {
				putc(iStop, fpOut);
			}
			pclose(fpShell);
		} else if (0 == strncmp(pc, acHead, sizeof(acHead)-1)) {
			if (fVerbose)
				fprintf(stdout, "%s: new header\n", progname);
			fNewHeader = fHeader;
			fflush(fpOut);
			if (stdout != fpOut)
				fclose(fpOut);
			if (fActive && NULL == (fpOut = fopen(pcTemp, "w"))) {
				fprintf(stderr, "%s: fopen: %s: %s\n", progname, pcTemp, strerror(errno));
				exit(2);
			}
		} else if (0 == strncmp(pc, acRem, sizeof(acRem)-1)) {
			if (fVerbose)
				fprintf(stdout, "%s: remove text\n", progname);
			fflush(fpOut);
dev_null:
			if (stdout != fpOut)
				fclose(fpOut);
			if (fActive && NULL == (fpOut = fopen(acNull, "w"))) {
				fprintf(stderr, "%s: fopen: %s: %s\n", progname, acNull, strerror(errno));
				exit(2);
			}
		} else if (0 == strncmp(pc, acMsg, sizeof(acMsg)-1)) {
			if (fVerbose)
				fprintf(stdout, "%s: message text\n", progname);
			fflush(fpOut);
			if (stdout != fpOut)
				fclose(fpOut);
			if (fActive && NULL == (fpOut = fopen(acTty, "w"))) {
				fprintf(stderr, "%s: fopen: %s: %s\n", progname, acTty, strerror(errno));
				exit(2);
			}
		}
	}

get_out:
	if ((FILE *)0 != fpIn && stdin != fpIn) {
		fclose(fpIn);
	}
	if ((FILE *)0 != fpOut && stdout != fpOut)
		fclose(fpOut);
	RemoveTemp();
}
