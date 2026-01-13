/* $Id: entomb.c,v 3.13 2008/11/17 18:54:38 ksb Exp $
 * Matthew Bradburn, Purdue University Computing Center			(mjb)
 * Greg Becker and Kevin Braunsdorf too				(becker) (ksb)
 * 
 * Copyright 1988, 1992 Purdue Research Foundation.  All rights reserved.
 */
#if !defined lint
static char *rcsid =
	"$Id: entomb.c,v 3.13 2008/11/17 18:54:38 ksb Exp $";
#endif

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/param.h>
#include <stdlib.h>

#include "machine.h"
#include "libtomb.h"
#include "debug.h"
#include "entomb.h"
#include "fslist.h"

#if defined(SYSV)
#include <sgsparam.h>
#endif

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if HAVE_DIRENT
#include <dirent.h>
#else
#include <sys/dir.h>
#endif

#if USE_FCNTL_H
#include <fcntl.h>
#endif

extern int errno;
#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif


/*
 * local data and functions
 */
static char acUid[16] = "-1";	/* chararcter string form of users's uid */
static int iUserUid = -1;	/* the uid of the invoker */
static int iUserGid = -1;	/* the gid of the invoker */
static int iTombUid = -1;	/* setuid? (for chowns) */
static int iTombGid = -1;	/* setgid gid */

/*
 * what to do the file corresponding to the given struct stat.	     (mjb/ksb)
 *
 * return any of:
 *	UNLINK		don't entomb at all, unlink original
 *	COPY		copy to the tomb, leave original alone
 *	RENAME		rename to tomb, original disappears
 *	COPY_UNLINK	copy to tomb, unlink the original
 */
static int
Action(iHow, pStatBuf)
int iHow;
struct stat *pStatBuf;
{
	/* if it isn't any of the above, just unlink it.  It might be a FIFO
	 * a device entry, a symlink, or somesuch.
	 * also trash zero length files.
	 */
	if (S_IFREG != (pStatBuf->st_mode&S_IFMT) || 0 == pStatBuf->st_size) {
		return UNLINK;
	}

	/* Simply rename or copy regular files with link count == 1.
	 * Copy and unlink hard links.  Otherwise, users could make a
	 * hard link to a file and get the link chowned to the tomb owner.
	 */
	if (COPY == iHow) {
		return COPY;
	}
	if (COPY_UNLINK == iHow || pStatBuf->st_nlink > 1) {
		return COPY_UNLINK;
	}
	return RENAME;
}


/* EntombInit -- record our uid's and gid's			(becker/ksb)
 * Call this function before any of the others in this file.
 */
int
EntombInit()
{
	static int tInit = 0;

	if (tInit) {
		return 0;
	}
	tInit = 1;

	if (-1 == (iUserUid = getuid())) {
		fprintf(stderr, "%s: getuid: %s\n", progname, strerror(errno));
		exit(NOT_ENTOMBED);
	}
	if (-1 == (iUserGid = getgid())) {
		fprintf(stderr, "%s: getgid: %s\n", progname, strerror(errno));
		exit(NOT_ENTOMBED);
	}
	if (-1 == (iTombUid = geteuid())) {
		fprintf(stderr, "%s: geteuid: %s\n", progname, strerror(errno));
		exit(NOT_ENTOMBED);
	}
	if (-1 == (iTombGid = getegid())) {
		fprintf(stderr, "%s: getegid: %s\n", progname, strerror(errno));
		exit(NOT_ENTOMBED);
	}
	return 1;
}


/* In the comments below the user ??? could be `joe' or `charon' or `root'
 * any should function in the code, each gives a different feature to the
 * entomb.  I suggest `joe' for NFS, charon for HPUX, and root for others.
 * (respectively; 2755 root.charon, 6555 charon.charon, 6755 root.charon).
 */

/* change effective group to charon				     (becker)
 * real `joe.group', effective `???.charon'
 */
static void
charon()
{
#if USE_SAVED_UID
	/* if we drop from root we cannot get back...
	 */
	if (0 != iUserUid) {
		setuid(iTombUid);
	}
	setgid(iTombGid);
#else
	/* if we can become charon and still get back to joe, do it
	 */
	if (0 == iTombUid || TOMB_UID == iTombUid || 0 == iUserUid) {
		(void)setreuid(iUserUid, TOMB_UID);
	} else if (iTombUid != iUserUid) {
		(void)setreuid(iUserUid, iTombUid);
	}
	/* if we can be group charon, do it
	 */
	if (0 == iTombUid || TOMB_GID == iTombGid || 0 == iUserUid) {
		(void)setregid(iUserGid, TOMB_GID);
	} else if (-1 == setregid(iUserGid, iTombGid)) {
		fprintf(stderr, "%s: setregid: %s\n", progname, strerror(errno));
		exit(NOT_ENTOMBED);
	}
#endif
}


/* change effective group to the user				     (becker)
 * real `???.charon', effective `joe.group'
 */
static void
user()
{
#if USE_SAVED_UID
	setgid(iUserGid);
	setuid(iUserUid);
#else
	if (iTombUid != iUserUid || 0 == iUserUid) {
		(void)setreuid(iTombUid, iUserUid);
	}
	if (-1 == setregid(iTombGid, iUserGid)) {
		fprintf(stderr, "%s: setregid: %s\n", progname, strerror(errno));
		exit(NOT_ENTOMBED);
	}
#endif
}

/* change effective user to root for chown, if we can			(ksb)
 * real `root.group', effective `root.charon'
 */
static int
root()
{
	register int iRet = -1;

	if (0 == iTombUid || 0 == iUserUid) {
		iRet = setuid(0);
	}
#if USE_SAVED_UID
	(void)setgid(iTombGid);
#else
	(void)setregid(iUserGid, iTombGid);
#endif
	return iRet;
}

/* change back to the original calling effective/real user/group	(ksb)
 * real `joe.group', effective `???.charon'
 */
static void
original()
{
#if USE_SAVED_UID
	(void)setgid(iUserGid);
	(void)setuid(iUserUid);
#else
	if (-1 == setreuid(iTombUid, iUserUid)) {
		fprintf(stderr, "%s: setreuid: %s\n", progname, strerror(errno));
		exit(1);
	}
	if (-1 == setregid(iTombGid, iUserGid)) {
		fprintf(stderr, "%s: setregid: %s\n", progname, strerror(errno));
		exit(1);
	}
#endif
}


/* create a tomb directory for the appropriate user		 (becker)
 * and return it's name as a pointer to static storage.  If we
 * can't create the tomb for some reason (perhaps there is no
 * tomb directory on this filesystem?), we return NULL.
 *
 *  We *must* be charon.charon or joe.charon for this call.
 */
static char *
setup_crypt(pcPath, pcDir)
char *pcPath, *pcDir;
{
	register int u;

	/* now append the user name (uid string) to the tomb directory
	 */
	(void)sprintf(pcPath, "%s/%s", pcDir, acUid);

	/* We must be in group charon to create a crypt in the tomb
	 */
	u = umask(0);
	if (-1 == mkdir(pcPath, 0777)) {
		if (EEXIST != errno) {
			fprintf(stderr, "%s: mkdir: %s: %s\n", progname, pcPath, strerror(errno));
			return (char *)0;
		}
	}
	(void)umask(u);

	return pcPath;
}



/* entomb a file as given				      (mjb/ksb/becker)
 *	pcHow - "rename" or "copy".  Renaming is for unlink() and rename(),
 *		copying is for the rest, which want to have a file to act on.
 *	pcFile - a relative or absolute path to the file to be entombed.
 *
 * returns:
 * 	NO_TOMB		no tombs on this filesystem; file not entombed
 *	0		no error; file entombed ok.
 *	NOT_ENTOMBED	an error occured; file not entombed.
 */
int
Entomb(iHow, pcFile)
int iHow;
char *pcFile;
{
	auto struct stat statBuf;/* used on pcFile			*/
	auto int iMode = 0;	/* the mode of the file to be entombed	*/
	auto struct timeval TV;	/* now -- for the tomb time		*/
	auto char acPath[MAXPATHLEN+2]; /* the tomb file we create	*/
	register int iRet;	/* keep track of if we've lost it	*/
	register int what_to_do;/* is UNLINK, COPY, COPY_UNLINK, RENAME	*/
	register TombInfo *pTI;	/* tomb info pointer			*/
	register char *pc;	/* temp for string manipulations	*/
	register int fdTo;	/* open fd on acPath			*/

	/* drop to the user to stat the target file
	 */
	user();

	/* If the file doesn't meet certain criteria,
	 * then we just say the tomb doesn't exist.
	 */
	iRet = STAT_CALL(pcFile, &statBuf);
	original();

	if (0 != iRet) {
		return NOT_ENTOMBED;
	}

	/* get the mode of the file to be entombed, and keep it for
	 * later.  It's stored in the tomb with these permissions,
	 * and we use them to modify the file when it's restored.  This
	 * allows us to give the same modes to restored files as they
	 * had before they were entombed.
	 * However, since unrm is only setgid charon, we have to make it
	 * at least group readable.
	 */
	iMode = (statBuf.st_mode & 0777) | 0440;

	/* Put file in file owners crypt.
	 */
	(void)sprintf(acUid, "%d", (int)statBuf.st_uid);

	/* should this file be entombed? If we don't trap it here
	 * we will fall harmlessly through the code below, just being
	 * safe...
	 */
	if (UNLINK == (what_to_do = Action(iHow, &statBuf))) {
		return NOT_ENTOMBED;
	}

	/* Get the tomb for this file system.
	 */
	if ((TombInfo *)0 == (pTI = FsList(iTombGid, statBuf.st_dev))) {
		return NO_TOMB;
	}

	/* Become charon and build a crypt for the file.
	 * Could be a plain file in the tomb dir with the wrong name.
	 */
	charon();
	if ((char *)0 == setup_crypt(acPath, pTI->pcDir)) {
		original();
		return NOT_ENTOMBED;
	}


	/* stick the basename of the file onto the tomb directory name
	 * so we have a target name the user might remember
	 */
	if ((char *)0 == (pc = strrchr(pcFile, '/'))) {
		(void)strcat(acPath, "/");
		(void)strcat(acPath, pcFile);
	} else {
		(void)strcat(acPath, pc);
	}

	/* As charon get a unique name in the tomb, either a fd or
	 * a hard link to the file depending on what-to-do.
	 * If we can't see both the file to entomb and the tomb		(ksb)
	 * at the same time fall back to copy-and-unlink.
	 */
	(void)gettimeofday(& TV, (struct timezone *)0);
	pc = &acPath[strlen(acPath)];
	for (;;) {
		(void)sprintf(pc, "@%ld", TV.tv_sec++);
		switch (what_to_do) {
		case COPY:
		case COPY_UNLINK:
			if (-1 != (fdTo = open(acPath, O_WRONLY|O_CREAT|O_EXCL, iMode))) {
				(void)fchmod(fdTo, iMode);
				break;
			}
			if (EEXIST == errno) {
				continue;
			}
			original();
                        return NOT_ENTOMBED;
		case RENAME:
			if (0 == link(pcFile, acPath)) {
				break;
			}
			if (EEXIST == errno) {
				continue;
			}
			if (EACCES == errno || EPERM == errno) {
				what_to_do = COPY_UNLINK;
				continue;
			}
			original();
			return NOT_ENTOMBED;
		case UNLINK:
			/* we really won't put it in the tomb, oh well
			 */
			/* we *could* record the major/minor of special
			 * or some such, so we could restore lost char/block
			 * specials.  But it is *much* more trouble than
			 * is it worth
			 */
			break;
		default:
			fprintf(stderr, "%s: internal error\n", progname);
			exit(NOT_ENTOMBED);
		}
		break;
	}

	/* try the operation(s) as the user
	 */
	user();

	if (COPY == what_to_do || COPY_UNLINK == what_to_do) {
		register int fdFrom;		/* file descriptors	*/
		register int iBytes;    	/* number of bytes read */
		auto char acBuf[BUFSIZ];	/* for copying files	*/


		if (-1 == (fdFrom = open(pcFile, O_RDONLY, 0))) {
			iRet = NOT_ENTOMBED;
		} else {
			/* copy the file
			 */
			while ((iBytes = read(fdFrom, acBuf, sizeof(acBuf))) > 0) {
				if (-1 == write(fdTo, acBuf, iBytes)) {
					iRet = NOT_ENTOMBED;
					break;
				}
			}
			if (-1 == iBytes) {
				iRet = NOT_ENTOMBED;
			}
			(void)close(fdFrom);
		}
		(void)close(fdTo);
	}

	if (0 == iRet && COPY != what_to_do) {
		iRet = unlink_r(pcFile);
	}

	/* If we crashed and burned? -- cleanup
	 * else if we're setuid root, then we can chown the file here
	 * to take it off the users quota.  Otherwise, preend
	 * will have to do the chown in a few minutes.
	 */
	if (0 != iRet) {
		charon();
		(void)unlink_r(acPath);
	} else if (-1 != root()) {
		(void)chmod(acPath, iMode);
		(void)chown(acPath, TOMB_UID, iTombGid);
	}

	/* put the effective user and group back -- just in case
	 */
	original();

	return iRet;
}
