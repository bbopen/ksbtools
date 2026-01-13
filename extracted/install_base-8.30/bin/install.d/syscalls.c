/*
 * Copyright 1990 Purdue Research Foundation, West Lafayette, Indiana
 * 47907.  All rights reserved.
 *
 * Written by Kevin S Braunsdorf, ksb@cc.purdue.edu, purdue!ksb
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. Neither the authors nor Purdue University are responsible for any
 *    consequences of the use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Credit to the authors and Purdue
 *    University must appear in documentation and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 */

/*
 * filer the unix system calls so they are more verbose or ineffective	(ksb)
 * under some options
 */
#if !defined(lint)
static char *rcsid = "$Id: syscalls.c,v 8.9 2009/12/08 22:58:14 ksb Exp $";
#endif	/* !lint */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <syslog.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <sysexits.h>

#include "machine.h"
#include "configure.h"
#include "syscalls.h"
#include "install.h"
#include "main.h"

#if FCNTL_IN_SYS
#include <sys/fcntl.h>
#else
#include <fcntl.h>
#endif

#if SIGNAL_IN_SYS
#include <sys/signal.h>
#else
#include <signal.h>
#endif

#if STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if HAVE_TIMEVAL
#include <sys/time.h>
#endif

#if HAVE_CSYMLINKS
#include <sys/universe.h>
#endif


#if !defined(BINCHGRP)
#define BINCHGRP	"/bin/chgrp"
#endif

static char acChgrp[] =		/* full path name to chgrp		*/
	BINCHGRP;


static int iSigsBlocked = 0;	/* count nested calls			*/

#if HAVE_SIGMASK
static int OldSigMask;		/* old signal mask			*/
#endif

/*
 * Mask all maskable signals					     (jsmith)
 * is we mask CHLD ranlib breaks (actually the shell does)
 */
void
BlockSigs()
{
	if (0 == iSigsBlocked++) {
#if HAVE_SIGMASK
		register int Mask;
		Mask = sigmask(SIGHUP)|sigmask(SIGTERM)|
			sigmask(SIGINT)|sigmask(SIGTSTP)|sigmask(SIGQUIT);
		OldSigMask = sigsetmask(Mask);
#else	/* BSD */
		register int	i;
		for (i = 0; i < NSIG; i++) {
			signal(i, SIG_IGN);
		}
#endif	/* do signal stuff */
	}
}


/*
 * Return the signal mask to what it was in before BlockSigs()	     (jsmith)
 */
void
UnBlockSigs()
{
	if (0 == --iSigsBlocked) {
#if HAVE_SIGMASK
		(void)sigsetmask(OldSigMask);
#else	/* BSD */
		register int i;

		for (i = 0; i < NSIG; i++) {
			signal(i, SIG_DFL);
		}
#endif	/* more signal stuff */
	}
}


/*
 * install suffers an internal error and exits abruptly			(ksb)
 * (really just a good break point for dbx/gdb/sdb)
 */
void
Die(pcError)
char *pcError;		/* error message, last gasp as we croak		*/
{
	(void)fprintf(stderr, "%s: internal error: %s\n", progname, pcError);
	exit(EX_SOFTWARE);
}


#define safecopy(Mdst, Msrc, Mfield, Mpc) \
	if ((char *)0 != Msrc->Mfield) { \
		Mdst->Mfield = Mpc; \
		(void)strcpy(pc, Msrc->Mfield); \
		Mpc = strchr(Mpc, '\000')+1; \
	} else { \
		Mdst->Mfield = (char *)0;\
	}


/*
 * this routine saves a group entry in malloced memory			(ksb)
 * simply free it when you are done with it
 */
struct group *
savegrent(pGR)
struct group *pGR;
{
	register struct group *pGRNew;
	register int size, i;
	register char *pc;
	auto char **ppc;

	if ((struct group *)0 == pGR) {
		return (struct group *)0;
	}

	size = sizeof(struct group);
	if ((char *)0 != pGR->gr_name)
		size += strlen(pGR->gr_name)+1;
	if ((char *)0 != pGR->gr_passwd)
		size += strlen(pGR->gr_passwd)+1;
	for (i = 0; (char *)0 != pGR->gr_mem[i]; ++i) {
		size += sizeof(char *) + strlen(pGR->gr_mem[i]) + 1;
	}
	size += sizeof(char *);

	if ((char *)0 == (pc = malloc(size))) {
		return (struct group *)0;
	}

	pGRNew = (struct group *)pc;
	pc += sizeof(struct group);

	ppc = (char **) pc;
	pc += (1+i) * sizeof(char *);

	safecopy(pGRNew, pGR, gr_name, pc);
	safecopy(pGRNew, pGR, gr_passwd, pc);

	pGRNew->gr_gid = pGR->gr_gid;

	pGRNew->gr_mem = ppc;
	for (i = 0; (char *)0 != pGR->gr_mem[i]; ++i) {
		*ppc++ = pc;
		(void)strcpy(pc, pGR->gr_mem[i]);
		pc = strchr(pc, '\000')+1;
	}
	*ppc = (char *)0;

	return pGRNew;
}


/*
 * this routine saves a passwd entry in malloced memory			(ksb)
 * simply free it when you are done with it
 */
struct passwd *
savepwent(pPW)
struct passwd *pPW;
{
	register struct passwd *pPWNew;
	register int size;
	register char *pc;

	if ((struct passwd *)0 == pPW) {
		return (struct passwd *)0;
	}
	size = sizeof(struct passwd);
	if ((char *)0 != pPW->pw_name)
		size += strlen(pPW->pw_name) +1;
	if ((char *)0 != pPW->pw_passwd)
		size += strlen(pPW->pw_passwd) +1;
#if HAVE_PWCOMMENT
	if ((char *)0 != pPW->pw_comment)
		size += strlen(pPW->pw_comment) +1;
#endif
	if ((char *)0 != pPW->pw_gecos)
		size += strlen(pPW->pw_gecos) +1;
	if ((char *)0 != pPW->pw_dir)
		size += strlen(pPW->pw_dir) +1;
	if ((char *)0 != pPW->pw_shell)
		size += strlen(pPW->pw_shell) +1;

	if ((char *)0 == (pc = malloc(size))) {
		return (struct passwd *)0;
	}
	pPWNew = (struct passwd *)pc;
	pc += sizeof(struct passwd);
	safecopy(pPWNew, pPW, pw_name, pc);
	safecopy(pPWNew, pPW, pw_passwd, pc);
	pPWNew->pw_uid = pPW->pw_uid;
	pPWNew->pw_gid = pPW->pw_gid;
#if HAVE_QUOTA
	pPWNew->pw_quota = pPW->pw_quota;
#endif

#if HAVE_PWCOMMENT
	safecopy(pPWNew, pPW, pw_comment, pc);
#endif
	safecopy(pPWNew, pPW, pw_gecos, pc);
	safecopy(pPWNew, pPW, pw_dir, pc);
	safecopy(pPWNew, pPW, pw_shell, pc);

	return pPWNew;
}

#undef	safecopy



/* ChMode()
 *	run chmod(2) on file, or trace doing so
 */
int
ChMode(pcFile, mMode)
char *pcFile;		/* pathname of file to change			*/
int mMode;		/* mode to change to				*/
{
	mMode = PERM_BITS(mMode);
	if (FALSE != fTrace) {
		(void)printf("%s: chmod %o %s\n", progname, mMode, pcFile);
	} else if (-1 == chmod(pcFile, mMode)) {
		(void)fprintf(stderr, "%s: chmod (%04o): %s: %s\n", progname, mMode, pcFile, strerror(errno));
		return -1;
	}
	return 0;
}


/* ChGroup()
 *	run chgrp(1) on file (for when we are NOT root)
 */
int
ChGroup(pcFile, pgroup)
char *pcFile;		/* pathname of file to change group of		*/
struct group *pgroup;	/* info on group				*/
{
	if (0 != RunCmd(acChgrp, pgroup->gr_name, pcFile)) {
		(void)fprintf(stderr, "%s: `%s %s %s\' failed\n", progname, acChgrp, pgroup->gr_name, pcFile);
		return -1;
	}
	return 0;
}


/* ChOwnGrp()
 *	Change the ownership and group of a file (must be root)
 */
int
ChOwnGrp(pcFile, powner, pgroup)
char *pcFile;		/* the pathname of the file to change		*/
struct passwd *powner;	/* info for owner of file			*/
struct group *pgroup;	/* info for group of file			*/
{
	if (FALSE != fTrace) {
		(void)printf("%s: chown %s %s\n", progname, powner->pw_name, pcFile);
		(void)printf("%s: chgrp %s %s\n", progname, pgroup->gr_name, pcFile);
	} else if (-1 == chown(pcFile, powner->pw_uid, pgroup->gr_gid)) {
		(void)fprintf(stderr, "%s: chown: %s: %s\n", progname, pcFile, strerror(errno));
		return -1;
	}
	return 0;
}


/*
 * ChTimeStamp()
 *	Change the time stamp of an installed file to match the original
 *	timestamp of the file to install
 */
void
ChTimeStamp(pcFile, pSTTimes)
char *pcFile; 		/* the pathname of the file to change		*/
struct stat *pSTTimes;	/* stat buffer containing timestamp of exemplar	*/
{
#if HAVE_TIMEVAL
	auto struct timeval tv[2];

	tv[0].tv_sec = pSTTimes->st_atime;
	tv[1].tv_sec = pSTTimes->st_mtime;
	tv[0].tv_usec = tv[1].tv_usec = 0;
	if (-1 == utimes(pcFile, tv)) {
		(void)fprintf(stderr, "%s: utimes: %s: %s\n", progname, pcFile, strerror(errno));
	}
#else /* BSD */
#if HAVE_UTIMBUF
	auto struct utimbuf utb;
	utb.actime = pSTTimes->st_atime;
	utb.modtime = pSTTimes->st_mtime;
	if (-1 == utime(pcFile, &utb)) {
		(void)fprintf(stderr, "%s: utime: %s: %s\n", progname, pcFile, strerror(errno));
	}
#else
	auto time_t tv[2];

	tv[0] = pSTTimes->st_atime;
	tv[1] = pSTTimes->st_mtime;
	if (-1 == utime(pcFile, tv)) {
		(void)fprintf(stderr, "%s: utime: %s: %s\n", progname, pcFile, strerror(errno));
	}
#endif
#endif	/* BSD */
}

/*
 * DoCopy()
 *	Copy the file "File" to file "Dest"
 */
int
DoCopy(File, Dest)
char *File;		/* the file to copy				*/
char *Dest;		/* the target (destination) file		*/
{
	register int fdSrc;	/* file descriptors			*/
	register int fdDest;	/* file to dopy to			*/
	register int n;		/* number of chars read			*/
	register int fStdin;	/* are we doing stdin			*/
	auto char acBlock[BLKSIZ];
	auto struct stat statb_File;


	fStdin = '-' == File[0] && '\000' == File[1];

	if (FALSE != fTrace) {
		if (fStdin)
			(void)printf("%s: cat - >%s\n", progname, Dest);
		else
			(void)printf("%s: cp %s %s\n", progname, File, Dest);
		return SUCCEED;
	}

	if (fStdin)
		fdSrc = fileno(stdin);
	else
#if HAVE_OPEN3
	if (-1 == (fdSrc = open(File, O_RDONLY, 0400))) {
		(void)fprintf(stderr, "%s: open: %s: %s\n", progname, File, strerror(errno));
	}
#else	/* old style open */
	if (-1 == (fdSrc = open(File, O_RDONLY))) {
		(void)fprintf(stderr, "%s: open: %s: %s\n", progname, File, strerror(errno));
	}
#endif	/* do an open */

	if (fstat(fdSrc, &statb_File) < 0) {
		(void)fprintf(stderr, "%s: fstat: %s: %s\n", progname, File, strerror(errno));
		if (!fStdin)
			(void)close(fdSrc);
		return FAIL;
	}

	/* we have to have rw- on the file to strip it later		(ksb)
	 */
	if (-1 == (fdDest = creat(Dest, (int)statb_File.st_mode|0600))) {
		(void)fprintf(stderr, "%s: create: %s: %s\n", progname, Dest, strerror(errno));
		if (!fStdin)
			(void)close(fdSrc);
		return FAIL;
	}

	/* Do the copy here
	 */
	while (0 != (n = read(fdSrc, acBlock, sizeof(acBlock)))) {
		if (-1 == n) {
			(void)fprintf(stderr, "%s: read: %s: %s\n", progname, File, strerror(errno));
			if (!fStdin)
				(void)close(fdSrc);
			(void)close(fdDest);
			(void)unlink(Dest);
			return FAIL;
		}
		if (write(fdDest, acBlock, n) != n) {
			(void)fprintf(stderr, "%s: write: %s: %s\n", progname, Dest, strerror(errno));
			if (!fStdin)
				(void)close(fdSrc);
			(void)close(fdDest);
			(void)unlink(Dest);
			return FAIL;
		}
	}

	if (!fStdin)
		(void)close(fdSrc);
	(void)close(fdDest);

	return SUCCEED;
}


/*
 * StrTail()
 *	return last element of a pathname
 */
char *
StrTail(pcPath)
register char *pcPath;	/* pathname to get tail of			*/
{
	register char *pcTemp;

	/* don't hand strlen() or strrchr() a null pointer
	 */
	if ((char *) NULL == pcPath) {
		return (char *)NULL;
	}

	/* make sure we weren't handed something like "/a/b/" or "/a/b//".
	 * If we were we don't want to return ++pcTemp
	 */
	while ('/' == *(pcTemp = (pcPath + strlen(pcPath)) - 1)) {
		if (pcTemp == pcPath)
			break;
		*pcTemp = '\000';
	}

	/* return last element
	 */
	if ((char *) NULL == (pcTemp = strrchr(pcPath, '/')))
		return pcPath;
	return ++pcTemp;
}


/* Call mktemp if fTrace is not set, else replace XXXX... with $$	(ksb)
 */
char *
Mytemp(pcFile)
char *pcFile;
{
	register char *pcRet;
	extern char *mktemp();

	if (FALSE != fTrace) {
		if ((char *)0 == (pcRet = strchr(pcFile, '\000'))) {
			Die("strchr: nil pointer");
		}
		while (pcRet > pcFile && 'X' == pcRet[-1])
			--pcRet;
		(void)strcpy(pcRet, "$$");
		pcRet = pcFile;
	} else if ((char *)0 == (pcRet = mktemp(pcFile))) {
		Die("mktemp: nil pointer");
	}
	return pcRet;
}


/* MungName()								(ksb)
 *	Mangle a pathname to make it unique (we hope)
 * (note that the pathname passed must be in a large buffer)
 */
void
MungName(pcName)
char	*pcName; 	/* the name of the file we want to change	*/
{
	register char	*pcPat = PATMKTEMP;

#if HAVE_SHORTNAMES
#if !defined(MAXNAMLEN)
#define MAXNAMLEN	14
#endif	/* max path component length */
	register char	*pcTemp;
	register int	iPatLen;

	/* Pathname components must be no longer than 14 chars on 2.9bsd
	 * systems.  If necessary truncate the filename after the first
	 * 8 chars so we can append "XXXXXX" for Mytemp()
	 */
	if ((pcTemp = strrchr(pcName, '/')) != (char *) 0) {
		pcName = ++pcTemp;
	}

	iPatLen = strlen(pcPat);
	if (strlen(pcName) + iPatLen > MAXNAMLEN) {
		*(pcName + (MAXNAMLEN - iPatLen)) = '\000';
	}

#endif	/* path name limits */

	if (MAXPATHLEN < (strlen(pcName) + strlen(pcPat))) {	/* sanity */
		(void)fprintf(stderr, "%s: unreasonably long pathname `%s\'\n", progname, pcName);
		exit(EX_OSERR);
	}

	/* This is real simple on reasonable operating systems
	 */
	(void)strcat(pcName, pcPat);
	(void)Mytemp(pcName);
}


#if HAVE_SLINKS
/*
 * CopySLink()
 *	Copy a symbolic link; here we know that pcLink is a symbolic link,
 *	and pcCopy needs to be clone of it.
 *
 * Side Effects
 *	We might MungName our second argument, if we have to.
 */
int
CopySLink(pcLink, pcCopy)
char *pcLink, *pcCopy;
{
	auto char acUcb[MAXPATHLEN+1];
	auto int iUcbLen;
#if HAVE_CSYMLINKS
	auto char acAtt[MAXPATHLEN+1];
	auto int iAttLen;
#endif	/* conditional symbolic links			*/

#if HAVE_CSYMLINKS
	if ((iAttLen = readclink(pcLink, acAtt, MAXPATHLEN, U_ATT)) <= 0) {
		goto std_link;
	} else if ((iUcbLen = readclink(pcLink, acUcb, MAXPATHLEN, U_UCB)) < 0) {
		(void)fprintf(stderr, "%s: readclink(ucb): %s: %s\n", progname, pcLink, strerror(errno));
		return FAIL;
	}
	acAtt[iAttLen] = '\000';
	acUcb[iUcbLen] = '\000';
	if ('/' != acAtt[0]) {
		if ('/' != acUcb[0]) {
			(void)fprintf(stderr, "%s: neither branch of `%s\' is a full path, link will not point the correct place in OLD\n", progname, pcLink);
		} else {
			(void)fprintf(stderr, "%s: att branch of `%s\' is not a full path, it will not point the correct place in OLD\n", progname, pcLink);
		}
	} else if ('/' != acUcb[0]) {
		(void)fprintf(stderr, "%s: ucb branch of `%s\' is not a full path, it will not point the correct place in OLD\n", progname, pcLink);
	}
	if (FALSE != fTrace) {
		(void)printf("%s: ln -c att=%s ucb=%s %s\n", progname, acAtt, acUcb, pcCopy);
		return SUCCEED;
	}
	if (0 == csymlink(acUcb, acAtt, pcCopy)) {
		return SUCCEED;
	}
	if (errno != EEXIST) {
		(void)fprintf(stderr, "%s: csymlink: %s: %s\n", progname, pcCopy, strerror(errno));
		return FAIL;
	}
	MungName(pcCopy);
	if (0 != csymlink(acUcb, acAtt, pcCopy)) {
		(void)fprintf(stderr, "%s: csymlink: %s: %s\n", progname, pcCopy, strerror(errno));
		return FAIL;
	}
	return SUCCEED;

std_link:
	/* dynix link code finds a nornal ucb link to copy -- fall into */;
#endif	/* might have to copy a csymlink	*/

	if ((iUcbLen = readlink(pcLink, acUcb, MAXPATHLEN)) < 0) {
		(void)fprintf(stderr, "%s: readlink: %s: %s\n", progname, pcLink, strerror(errno));
		return FAIL;
	}

	acUcb[iUcbLen] = '\000';
	if ('/' != acUcb[0]) {
		(void)fprintf(stderr, "%s: link `%s\' is not a full path, it will not point the correct place in OLD\n", progname, pcLink);
	}
	if (FALSE != fTrace) {
		(void)printf("%s: ln -s %s %s\n", progname, acUcb, pcCopy);
		return SUCCEED;
	}
	if (0 == symlink(acUcb, pcCopy)) {
		return SUCCEED;
	}
	if (errno != EEXIST) {
		(void)fprintf(stderr, "%s: symlink: %s: %s\n", progname, pcCopy, strerror(errno));
		return FAIL;
	}
	MungName(pcCopy);
	if (0 != symlink(acUcb, pcCopy)) {
		(void)fprintf(stderr, "%s: symlink: %s: %s\n", progname, pcCopy, strerror(errno));
		return FAIL;
	}
	return SUCCEED;
}
#endif	/* don't handle links at all */


/*
 * Rename()
 *	Simulate the vax rename() syscall using link/unlink, or fTrace
 */
int
Rename(pcOrig, pcNew, pcNote)
char *pcOrig;		/* the original file name			*/
char *pcNew;		/* the name file will have after the rename	*/
char *pcNote;		/* tell the user always?			*/
{
	register int fRet;

	BlockSigs();

	if ((char *)0 != pcNote) {
		(void)printf("%s: %s %s to %s\n", progname, pcNote, pcOrig, pcNew);
	}

	fRet = SUCCEED;
	if (FALSE != fTrace) {
		(void)printf("%s: mv %s %s\n", progname, pcOrig, pcNew);
	} else {
#if HAVE_RENAME
		if (-1 == rename(pcOrig, pcNew)) {
			(void)fprintf(stderr, "%s: rename: %s to %s: %s\n", progname, pcOrig, pcNew, strerror(errno));
			fRet = FAIL;
		}
#else	/* need to invent rename	*/
		(void)unlink(pcNew);
		if (-1 == link(pcOrig, pcNew)) {
			(void)fprintf(stderr, "%s: link: %s to %s: %s\n", progname, pcOrig, pcNew, strerror(errno));
			fRet = FAIL;
		} else if (-1 == unlink(pcOrig)) {
			(void)fprintf(stderr, "%s: unlink: %s: %s\n", progname, pcOrig, strerror(errno));
			fRet = FAIL;
		}
#endif	/* have rename to call		*/
	}

	UnBlockSigs();

	return fRet;
}

/*
 * IsDir()
 *	find out if a file is a directory, or a symlink to a directory
 */
int
IsDir(dest)
char *dest;
{
	auto struct stat statb;

	if (-1 == stat(dest, &statb)) {
		return FALSE;
	}

	if (S_IFDIR == (statb.st_mode & S_IFMT)) {
		return TRUE;
	}
	return FALSE;
}


/*
 * MyWait()
 *	loop waiting until given child exits, return the childs status
 */
int
MyWait(pid)
int pid;		/* process id to wait for			*/
{
	auto int wpid;
#if HAVE_UWAIT
	auto union wait	status;
#else	/* !BSD */
	auto int status;
#endif	/* set up for a wait */

	while ((wpid = wait(&status)) != pid) {
		if (wpid == -1)
			break;
	}

	if (wpid == -1) {
		/* this happens on the ETA, where did the kid go?
		 */
		if (ECHILD == errno)
			return 0;
		(void)fprintf(stderr, "%s: wait: %s\n", progname, strerror(errno));
		return 1;
	}

#if HAVE_UWAIT
	return status.w_status;
#else	/* BSD */
	return status;
#endif	/* return value from wait */
}


/*
 * RunCmd()
 *	Vfork's & exec's a command and wait's for it to finish
 *	we return 1 for FAIL here, not FAIL, caller compares to 0
 *	(as this is the sh(1) convention).
 */
int
RunCmd(cmd, arg1, arg2)
char *cmd;		/* full pathname of the command			*/
char *arg1;		/* first argument to cmd			*/
char *arg2;		/* second argument to cmd (can be nil)		*/
{
	auto char *arg0;	/* basename of cmd			*/
	register int pid;

	arg0 = StrTail(cmd);
	if (FALSE != fTrace) {
		(void)printf("%s: %s %s", progname, arg0, arg1);
		if ((char *)0 != arg2)
			(void)printf(" %s", arg2);
		(void)fputc('\n', stdout);
		return 0;
	}

	(void)fflush(stdout);
	(void)fflush(stderr);
	switch ((pid = vfork())) {
	case 0:		/* child					*/
		execl(cmd, arg0, arg1, arg2, (char *)0);
		(void)fprintf(stderr, "%s: execl: `%s\': %s\n", progname, cmd, strerror(errno));
		exit(EX_OSERR);
	case -1:	/* error					*/
		(void)fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		exit(EX_TEMPFAIL);
	default:	/* parent					*/
		break;
	}

	return MyWait(pid);
}
