/* the real brains of maketd are in this file
 * $Id: maketd.c,v 4.31 1997/10/01 20:49:14 ksb Exp $
 */
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>

#include "machine.h"
#include "main.h"
#include "maketd.h"
#include "srtunq.h"
#include "abrv.h"

extern int errno;
extern char *mktemp();

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if !defined(F_OK)
#define F_OK	0
#endif

char
	*pchBackup;	/* the backup file name				*/
char
	*pchOrig = (char *)0; /* put the back back as this		*/
DependInfo
	*pDIList;

static struct srtent
	incld;
static FILE
	*fpPipe;	/* pipe file pointer for preproc		*/
static char
	*apcInc[] = {
		"/usr/include",
		"/usr/include/sys",
		"/usr/local/lib/gcc-lib",
		"/usr/local/lib/mkcmd",
		(char *)0
	},
	acDepNote[] =
		"# DO NOT DELETE THIS LINE - see .depend for mkdep output\n",
	acDepline[] =
		"# DO NOT DELETE THIS LINE - maketd DEPENDS ON IT\n",
	acStartdep[] =
		"# DO NOT DELETE THIS LINE",
	acTrailer[] =
		"\n# *** Do not add anything here - It will go away. ***\n",
	acStopdep[] =
		 "# *** Do not add anything here";

/* Back up the makefile/.depend into the backup position		(ksb)
 */
void
BackUp(pcWhich)
char *pcWhich;
{
	register unsigned len;
	if ((char *)0 == exten) {
		exten = STRSAVE(".bak");
	}

	len = strlen(pcWhich)+strlen(exten)+2;
	pchBackup = malloc(len);
	strcpy(pchBackup, pcWhich);
	if ('.' != *exten) {
		strcat(pchBackup, ".");
	}
	strcat(pchBackup, exten);

	/* Remove the old backup file */
	if (0 == access(pchBackup, F_OK) && 0 != unlink(pchBackup)) {
		fprintf(stderr, "%s: unlink: %s: %s\n", progname, pchBackup, strerror(errno));
		exit(3);
	}

	/* Move current makefile to backup file */
	if (0 != link(pcWhich, pchBackup)) {
		fprintf(stderr, "%s: link: %s to %s: %s\n", progname, pcWhich, pchBackup, strerror(errno));
		exit(4);
	}

	pcOrig = pcWhich;
	if (0 != unlink(pcWhich)) {
		fprintf(stderr, "%s: unlink: %s: %s\n", progname, pcWhich, strerror(errno));
		RestoreFiles();
	}
}

/* Given the info about the file, figure out what the destination name
 * should be on the dependancy line.
 */
char *
DestName(pDIFile, pcName)
DependInfo *pDIFile;
char *pcName;
{
	register char *pch = (char *)0;

	/* find a prefix:
	 * the -o (destdir) flag has precedence over the -n or -l flags;
	 * then the non-local o option;
	 * else we don't want one
	 */
	if ((char *)0 != pDIFile->destdir) {
		pch = strrchr(strcpy(pcName, pDIFile->destdir), '\000');
		if ('/' != *--pch) {
			*++pch = '/';
			*++pch = '\000';
		}
	} else if (0 != pDIFile->localo) {
		strcpy(pcName, pDIFile->filename);
		if ((char *)0 != (pch = strrchr(pcName, '/'))) {
			pch[1] = '\000';
		}
	} else {
		pcName[0] = '\000';
	}

	/* find a basename
	 * when they supplied the basename, trust them
	 * else strip off any leading pathname to the file and save it in pcName
	 *	(but get rid of the suffix too)
	 * thus pcName contains only the basename
	 */
	if ((char *)0 != pDIFile->basename) {
		strcat(pcName, pDIFile->basename);
	} else {
		if ((char *)0 == (pch = strrchr(pDIFile->filename, '/'))) {
			pch = pDIFile->filename;
		} else {
			++pch;
		}
		strcat(pcName, pch);
		if ((char *)0 != (pch = strrchr(pcName, '.'))) {
			*pch = '\000';
		}
	}

	/* if this is not a binary dependancy, add a suffix to it
	 */
	if (1 != pDIFile->binarydep) {
		if ((char *)0 != pDIFile->suffix) {
			strcat(pcName, pDIFile->suffix);
		} else {
			strcat(pcName, ".o");
		}
	}
	return pcName;
}

/* DoReplace
 * This is a routine that goes through the current dependancies (if any)
 * and spits out the ones that are NOT being replaced. If the shortening
 * of include file names is turned off, it will spit out the currently
 * defined vars, otherwise, it lets them be rebuilt.
 */
void
DoReplace(fpDest, fpSource)
FILE *fpDest, *fpSource;
{
	register DependInfo *pDI;
	register int found = 0, cont = 0;
	register int len;
	auto char acRepl[1024];
	auto char acLine[BUFSIZE];

	while ((char *)0 != fgets(acLine, sizeof(acLine), fpSource)) {
		if (0 != cont) {
			if (0 == found) {
				fputs(acLine, fpDest);
			}
			cont = '\\' == acLine[strlen(acLine) - 2];
			continue;
		}

		found = cont = 0;
		if ('\n' == acLine[0]) {
			continue;
		}

		/* If we are not shortening the include file names on this
		 * replacement, we at least have to spit out the old
		 * abbreviations
		 */
		if (isupper(acLine[0]) && '=' == acLine[1]) {
			register char *pcTemp;
			pcTemp  = strrchr(acLine, '\n');
			if ((char *)0 == pcTemp) {
				fprintf(stderr, "%s: internal error or line too long\n", progname);
				RestoreFiles();
			}
			*pcTemp = '\000';
			if ((char *)0 == srtin(&abrv, hincl(acLine+2), lngsrt)) {
				OutOfMemory();
			}
			if (0 != verbose)
				fprintf(stderr, "%s: inserted abbr %s\n", progname, acLine+2);
			continue;
		}
		if (0 == strncmp(acStopdep, acLine, sizeof(acStopdep)-1)) {
			break;
		}
		for (pDI = pDIList; (DependInfo *)0 != pDI; pDI = pDI->next) {
			DestName(pDI, acRepl);
			len = strlen(acRepl);
			found = ':' == acLine[len] && 0 == strncmp(acRepl, acLine, len);
			if (found)
				break;
		}
		if (0 == found && '#' != acLine[0]) {
			fputc('\n', fpDest);
			fputs(acLine, fpDest);
		}
		cont = '\\' == acLine[strlen(acLine) - 2];
	}
}

/* output a user recipe expanded file this file				(ksb)
 */
void
Expand(pcRecipe, pDI, fpTo, pcPrefix)
char *pcRecipe, *pcPrefix;
DependInfo *pDI;
FILE *fpTo;
{
	register int i, num;
	register char chLast = '\n';
	register char *pc;
	auto char acf[1025];
	auto char acF[1025];
	auto char
		*tp,	/* the / in %o/%t%s		*/
		*sp,	/* the . in %s, or a "\000"	*/
		*Tp,	/* the / in %o/%t%s		*/
		*Sp;	/* the . in %s, or a "\000"	*/

	pc = strrchr(pDI->filename, '/');
	if ((char *)0 == pc) {
		strcpy(acf, ".");
		strcpy(acF, ".");
	} else {
		*pc = '\000';
		strcpy(acf, pDI->filename);
		strcpy(acF, pDI->filename);
		*pc = '/';
	}
	if ((char *)0 != pDI->destdir) {
		strcpy(acf, pDI->destdir);
	}

	tp = strrchr(acf, '\000');
	tp[0] =  '/';
	Tp = strrchr(acF, '\000');
	Tp[0] =  '/';
	if ((char *)0 == pc) {
		strcpy(tp+1, pDI->filename);
		strcpy(Tp+1, pDI->filename);
	} else {
		strcpy(tp+1, pc+1);
		strcpy(Tp+1, pc+1);
	}
	if ((char *)0 != pDI->basename) {
		strcpy(tp+1, pDI->basename);
	}

	sp = strrchr(tp, '.');
	if ((char *)0 == sp)
		sp = strrchr(tp, '\000');
	Sp = strrchr(Tp, '.');

	if ((char *)0 != pDI->suffix) {
		strcpy(sp, pDI->suffix);
	} else if (1 == pDI->binarydep) {
		sp[0] = '\000';
	} else {
		strcpy(sp, ".o");
	}

	while (*pcRecipe) {
		if ('\n' == chLast) {
			fputs(pcPrefix, fpTo);
		}
		chLast = *pcRecipe;
		switch (*pcRecipe) {
		case '%':
			/* %f: %F
			 * %o/%t.%s: %O/%T.%S
			 * %%
			 */
			switch (*++pcRecipe) {
			/* target */
			case 'o':	/* directory part	*/
				*tp = '\000';
				fputs(acf, fpTo);
				*tp = '/';
				break;

			case 't':
				if ('\000' == *sp) {
					fputs(tp+1, fpTo);
				} else {
					i = *sp;
					*sp = '\000';
					fputs(tp+1, fpTo);
					*sp = i;
				}
				break;

			case 's':
				fputs(sp, fpTo);
				break;

			case 'f':
				fputs(acf, fpTo);
				break;

			/* source */
			case 'O':
				*Tp = '\000';
				fputs(acF, fpTo);
				*Tp = '/';
				break;

			case 'T':
				if ((char *)0 == Sp) {
					fputs(Tp+1, fpTo);
				} else {
					*Sp = '\000';
					fputs(Tp+1, fpTo);
					*Sp = '.';
				}
				break;

			case 'S':
				if ((char *)0 != Sp)
					fputs(Sp, fpTo);
				break;

			case 'F':
				fputs(acF, fpTo);
				break;

			/* global */
			case 'L':
				if ((char *)0 != pDI->inlib)
					fputs(pDI->inlib, fpTo);
				break;

			case 'I':
				if ((char *)0 != pDI->cppflags)
					fputs(pDI->cppflags, fpTo);
				break;

			case '%':
			default:	/* unrecognized chars are copied thru */
				fputc(*pcRecipe, fpTo);
				break;
			}
			pcRecipe++;
			break;

		case '\\':
			switch (*++pcRecipe) {
			case '\n':	/* how would this happen? */
				++pcRecipe;
				break;
			case 'a':
				fputc('\007', fpTo);
				++pcRecipe;
				break;
			case 'n':	/* newline */
				fputc('\n', fpTo);
				chLast = '\n';
				++pcRecipe;
				break;
			case 't':
				fputc('\t', fpTo);
				++pcRecipe;
				break;
			case 'b':
				fputc('\b', fpTo);
				++pcRecipe;
				break;
			case 'r':
				fputc('\r', fpTo);
				++pcRecipe;
				break;
			case 'f':
				fputc('\f', fpTo);
				++pcRecipe;
				break;
			case 'v':
				fputc('\013', fpTo);
				++pcRecipe;
				break;
			case '\\':
				++pcRecipe;
			case '\000':
				fputc('\\', fpTo);
				break;

			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
				num = *pcRecipe++ - '0';
				for (i = 0; i < 2; i++) {
					if (! isdigit(*pcRecipe)) {
						break;
					}
					num <<= 3;
					num += *pcRecipe++ - '0';
				}
				fputc(num, fpTo);
				chLast = num;
				break;
			case '8': case '9':
				/* 8 & 9 are bogus octals,
				 * cc makes them literals
				 */
				/*fallthrough*/
			default:
				fputc(*pcRecipe++, fpTo);
				break;
			}
			break;
		default:
			fputc(*pcRecipe, fpTo);
			pcRecipe++;
			break;
		}
	}

	if ('\n' != chLast)
		fputc('\n', fpTo);
}

/* file is in the "system" part of the file system for -a		(ksb)
 */
static int
IsSystem(pcFile)
char *pcFile;
{
	register char **ppcName;
	register int l;

	for (ppcName = apcInc; (char *)0 != *ppcName; ++ppcName) {
		l = strlen(*ppcName);
		if (0 == strncmp(*ppcName, pcFile, l) && '/' == pcFile[l])
			return 1;
	}
	return 0;
}

/* generate the depend info for a given file				(ksb)
 */
static void
DoDepend(pDI, fpOut)
DependInfo *pDI;
FILE *fpOut;
{
	register int lineend, i, linenew, moption, ch;
	register char *p, *q;
	auto int complen;
	auto char acLine[BUFSIZE], acCmd[1024+400];
	auto char acFile[1025], acThis[1025], *pcName;
	auto FILE *fpCmd;

	/* compute the dest name and save it, we want this one for a while
	 */
	pcName = DestName(pDI, acThis);
	if (0 != verbose) {
		fprintf(stderr, "%s: working on %s\n", progname, pcName);
	}

	/* with the help of a little conditional compilation
	 * set up the command string
	 */
	mktemp(strcpy(acFile, "/tmp/mtdXXXXXX"));
	if ((FILE *)0 == (fpCmd = fopen(acFile, "w"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, acFile, strerror(errno));
		RestoreFiles();
	}
	Expand(pDI->preprocessor, pDI, fpCmd, "");
	fclose(fpCmd);

	if (0 != verbose) {
		fprintf(stderr, "%s: shell command: (in %s)\n", progname, acFile);
		sprintf(acCmd, "cat %s", acFile);
		system(acCmd);
	}

	(void)sprintf(acCmd, "sh <%s", acFile);
	if ((FILE *)0 == (fpPipe = popen(acCmd, "r"))) {
		fprintf(stderr, "%s: popen: %s: %s\n", progname, acCmd, strerror(errno));
		RestoreFiles();
	}

	/* Start with a fresh list of dependancy files */
	srtfree(&incld);

	/* check for cpp output with or without -M option */
	if ('#' == (ch = fgetc(fpPipe))) {
		moption = 0;
	} else {
		moption = 1;
	}
	if (EOF == ungetc(ch, fpPipe)) {
		fprintf(stderr, "%s: trouble determining -M status\n", progname);
		RestoreFiles();
	}

	/* we follow two sets of rules for cpp output and gcc output
	 */
	if (0 == moption) {
		while ((char *)0 != fgets(acLine, sizeof(acLine), fpPipe)) {
			/* An include statement starts with a # */
			if ('#' != acLine[0])
				continue;

			/* eat #ident lines (mostly a sysv thing) */
			p = acLine;
			do {
				++p;
			} while (' ' == *p || '\t' == *p);
			if (! isdigit(*p)) {
				continue;
			}

			/* Find the first double quote and go one past it
			 * and make the path pretty
			 */
			if ((char *)0 == (p = strchr(p, '"')))
				continue;
			p = hincl(p+1);

			if ((char *)0 != (q = strchr(p, '"')))
				*q = '\000';

			/* skip over:
		 	 *   - the system include path (if -a all dependancies
			 *	are NOT to be made)
			 *   - the mkcmd fake file name
			 *   - ??? LLL
		 	 */
			if (0 == alldep && IsSystem(p))
				continue;
			if (0 == strcmp(p, "mkcmd_generated_main"))
				continue;

			/* Insert it into our list */
			if ((char *)0 == srtin(&incld, p, strcmp))
				OutOfMemory();
			if (0 != verbose)
				fprintf(stderr, "%s: inserted %s\n", progname, p);
		}
	} else {
		linenew = 1;
		while ((char *)0 != fgets(acLine, sizeof(acLine), fpPipe)) {
			if (1 == linenew) {
				if ((char *)0 == (p = strchr(acLine, ':'))) {
					fprintf(stderr, "%s: preprocessor output missing colon\n", progname);
					RestoreFiles();
				} else {
					++p;           /* Skip colon */
				}
				if (' ' != *p++) {
					fprintf(stderr, "%s: preprocessor output missing space\n", progname);
					RestoreFiles();
				}
			} else {
				/* no colon is alright as well -- which
				 * would be a continued line from gcc
				 */
				p = acLine;
			}
			while (isspace(*p)) {
				++p;
			}

			/* check to see if the next line is a continuation
			 */
			if ((char *)0 != (q = strchr(p, '\n'))) {
				if (q > p && '\\' == q[-1]) {
					--q;
					linenew = 0;
					moption = 2;
				} else {
					linenew = 1;
				}
				*q = '\000';
			}

			for (q = p; '\000' != *p; p = q) {
				while ('\000' != *q && !isspace(*q))
					++q;
				while (isspace(*q))
					*q++ = '\000';

				/* Make pathname pretty */
				p = hincl(p);

				/* If all dependancies are NOT to be made,
				 * skip over the basic include path
		 		 */
				if (0 == alldep && IsSystem(p))
					continue;
				if (0 == strcmp(p, "mkcmd_generated_main"))
					continue;

				/* Insert it into our list */
				if ((char *)0 == srtin(&incld, p, strcmp)) {
					OutOfMemory();
				}
				if (0 != verbose)
					fprintf(stderr, "%s: inserted %s\n", progname, p);
			}
		}
	}

	if (0 != verbose) {
		if (0 == moption)
			fprintf(stderr, "%s: read straight cpp\n", progname);
		else if (1 == moption)
			fprintf(stderr, "%s: read cpp -M output\n", progname);
		else
			fprintf(stderr, "%s: read gcc-cpp -M output\n", progname);
	}

	/* Initialize so we can use srtgets */
	srtgti(&incld);

	lineend = MAXCOL+1;		/* Force a newline in the output */
	/* Write out the entries */
	while ((char *)0 != (p = srtgets(&incld))) {
		/* set i = found index or MXABR */
		if (0 != shortincl && MXABR != (i = findabr(p))) {
			p += abrvlen[i];
			if ('/' != p[0] && '\000' != p[0]) {
				complen = 4;
			} else {
				complen = 2;
			}
		} else {
			complen = 0;
		}

		complen += strlen(p);
		if (MAXCOL <= lineend + complen) {
			if ((char *)0 != pcName) {
				/* 2 for the colon and space */
				lineend = strlen(pcName) + 2;
				if ((char *)0 != pDI->inlib) {
					/* + parens  and lib pcName */
					lineend += 2 + strlen(pDI->inlib);
					fprintf(fpOut, "\n%s(%s): ", pDI->inlib, pcName);
				} else {
					fprintf(fpOut, "\n%s: ", pcName);
				}
				pcName = (char *)0;
				if (MAXCOL <= lineend + complen) {
					lineend = 8;
					fprintf(fpOut, "\\\n\t");
				}
			} else {
				lineend = 8;
				fprintf(fpOut, " \\\n\t");
			}
		} else {
			++lineend;
			fputc(' ', fpOut);
		}

		if (0 != shortincl && MXABR != i) {
			if ('/' != p[0] && '\000' != p[0]) {
				fprintf(fpOut, "${%c}", 'A' + i);
			} else {
				fprintf(fpOut, "$%c", 'A' + i);
			}
		}
		fputs(p, fpOut);
		lineend += complen;
	}
	fputc('\n', fpOut);
	pclose(fpPipe);
	(void)unlink(acFile);
	if ((char *)0 != pDI->explicit) {
		Expand(pDI->explicit, pDI, fpOut, "\t");
	}
}

/* This part is the actual work-horse of the program.			(ksb)
 * All of the options have been processed at this point, all we have to do
 * is interpret their meanings.
 */
int
MakeTD(pcMakefile, pcDepends)
char *pcMakefile, *pcDepends;
{
	register DependInfo *pDI;
	register int fCont;
	register char *pchTemp;
	register FILE *fpBuild, *fpBackup;
	auto char sbLine[BUFSIZE];
	auto struct stat stOld;

	/* Find out which makefile is to be used. First, if the user
	 * specifies a makefile, use that. If he doesn't, check and
	 * see if 'makefile' exists. Finally, if that doesn't check
	 * out, see if 'Makefile' exists.
	 */
	if ((char *)0 == pcMakefile) {
		pcMakefile = STRSAVE("makefile");
		if (0 != access(pcMakefile, F_OK)) {
			pcMakefile[0] = 'M';
			if (0 != access(pcMakefile, F_OK)) {
				fprintf(stderr, "%s: cannot find makefile\n", progname);
				exit(1);
			}
		}
	} else if (0 != access(pcMakefile, F_OK)) {
		fprintf(stderr, "%s: access: %s: %s\n", progname, pcMakefile, strerror(errno));
		exit(1);
	}

	/* Now backup the makefile to a backup file and unlink the current one
	 */
	if (use_stdout) {
		pchBackup = pcMakefile;
	} else {
		BackUp(pcMakefile);
	}

	/* Now open the destination makefile, copy over all of the current
	 * makefile up to the '# DO NOT DELETE THIS LINE' line (or the
	 * end-of-file, whichever comes first), and then continue with
	 * processing.
	 */
	if ((FILE *)0 == (fpBackup = fopen(pchBackup, "r"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, pchBackup, strerror(errno));
		RestoreFiles();
	}
	if (-1 == fstat(fileno(fpBackup), &stOld)) {
		fprintf(stderr, "%s: fstat: %s(%d): %s\n", progname, pchBackup, fileno(fpBackup), strerror(errno));
		RestoreFiles();
	}

	if (0 != use_stdout) {
		fpBuild = stdout;
	} else if ((FILE *)0 == (fpBuild = fopen(pcMakefile, "w"))) {
		fprintf(stderr, "%s fopen: %s: %s\n", progname, pcMakefile, strerror(errno));
		RestoreFiles();
	} else {
		/* chmod the Makefile LLL use fchmod */
		if (-1 == chmod(pcMakefile, (stOld.st_mode & 07777))) {
			fprintf(stderr, "%s: chmod: %s: %s\n", progname, pcMakefile, strerror(errno));
			RestoreFiles();
		}
	}

	/* clever use of continuations used to hose us over
	 */
	fCont = 0;
	while ((char *)0 != fgets(sbLine, sizeof(sbLine), fpBackup) && (0 != fCont ||
		0 != strncmp(acStartdep, sbLine, sizeof(acStartdep)-1))) {
		pchTemp = strrchr(sbLine, '\\');
		if ((char *)0 != pchTemp && '\n' == pchTemp[1]) {
			fCont = 1;
		} else {
			if (0 == fCont)
				srchincl(sbLine);

			fCont = 0;
		}
		if (0 == use_stdout) {
			(void)fputs(sbLine, fpBuild);
		}
	}

	/* If we need the header put in, do it now
	 */
	if (fMkdep) {
		(void)fputs(acDepNote, fpBuild);
		fclose(fpBuild);
		if ((char *)0 == pcDepends || '\000' == *pcDepends) {
			RestoreFiles();
		}
		if ((FILE *)0 == (fpBuild = fopen(pcDepends, "w"))) {
			fprintf(stderr, "%s fopen: %s: %s\n", progname, pcDepends, strerror(errno));
			RestoreFiles();
		}
	} else if (0 == use_stdout || 0 != force_head) {
		(void)fputs(acDepline, fpBuild);
	}

	/* Put out the abbreviations table
	 * we should only put them out if used?
	 */
	if (0 != alldep) {
		register char **ppc;
		for (ppc = apcInc; (char *)0 != *ppc; ++ppc) {
			if (0 != verbose)
				fprintf(stderr, "%s: all depends inserts %s\n", progname, *ppc);
			if ((char *)0 == srtin(&abrv, hincl(*ppc), lngsrt))
				OutOfMemory();
		}
	}

	if (0 != shortincl) {
		abrvsetup(fpBuild);
	}

	/* Check to see if we have to replace the files instead of just
	 * recreating all dependancies
	 */
	if (0 != replace && 0 == use_stdout) {
		DoReplace(fpBuild, fpBackup);
	} else {
		fCont = 0;
		while ((char *)0 != fgets(sbLine, sizeof(sbLine), fpBackup) && (0 != fCont || 0 != strncmp(acStopdep, sbLine, sizeof(acStopdep)-1))) {
			pchTemp = strrchr(sbLine, '\\');
			fCont = (char *)0 != pchTemp && '\n' == pchTemp[1];
		}
	}

	/* Now, after we have done everything that needs to be done except
	 * putting out the dependancies for the list of files we have, lets
	 * go ahead and do that! First we have to initialize our sorted
	 * list.
	 */
	srtinit(&incld);	/* Initialize sorted list of files */
	for (pDI = pDIList; (DependInfo *)0 != pDI; pDI = pDI->next) {
		DoDepend(pDI, fpBuild);
	}

	/* Put out the trailer if needed
	 * and copy the rest of the makefile
	 */
	if (0 == use_stdout || 0 != force_head) {
		fputs(acTrailer, fpBuild);
	}
	while ((char *)0 != fgets(sbLine, sizeof(sbLine), fpBackup)) {
		static int fTailErr = 1;
		if (fTailErr) {
			fprintf(stderr, "%s: copied trailing junk on end of %s\n", progname, pchBackup);
			fTailErr = 0;
		}
		(void)fputs(sbLine, fpBuild);
	}

	/* close all those files we opened
	 */
	fclose(fpBackup);
	if (stdout != fpBuild) {
		fclose(fpBuild);
	}

#ifdef DEL_BACKUP
	if ((char *)0 != pcOrig && 0 != unlink(pchBackup)) {
		fprintf(stderr, "%s: unlink: %s: %s\n", progname, pchBackup, strerror(errno));
		exit(1);
	}
#endif /* DEL_BACKUP */
	exit(0);
}
