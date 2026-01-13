/* $Id: control.c,v 2.10 2000/07/29 19:07:09 ksb Exp $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>

#include "main.h"
#include "machine.h"
#include "glob.h"
#include "ckpass.h"

extern char *calloc();
extern char *getpass();
static int alldigits();

char acOwners[] = OWNERSFILE;

/* Prompt the user for their password if they haven't already		(bj)
 * entered it.
 *
 * For maintainers of large software packages (eg TeX), we let
 * them get by without a password if they're in the group of the
 * .owners file and it is setgid.  The intent is that they newgrp
 * to the group for the time it takes to install, sort of like su'ing.
 */
/* XXX gid_t u, uid_t */
static int iOwnerFileGid = -1;
static int iOwnerFileUid = -1;

static int
CheckAccess(iRealUid, iRealGid, gidset, ngroups)
int iRealUid, iRealGid;
int *gidset;
int ngroups;
{
	static int fDoneIt = 0;
	register char *pcPass;
	auto char acBuf[100];
	auto char acSalt[3];
	register struct passwd *pwdSuper;

	/* Don't set fDoneIt in the two checks below because we're paranoid
	 * that someone in the future will call ParseOwners multiple times.
	 */
	if (-1 != iOwnerFileGid) {
		register int i;

		if (iRealGid == iOwnerFileGid)
			return 1;
		for (i = 0; i < ngroups; ++i)
			if (iOwnerFileGid == gidset[i])
				return 1;
	}
	if (-1 != iOwnerFileUid && iRealUid == iOwnerFileUid) {
		return 1;
	}

	/* Don't ask for the password again, our real uid didn't change,
	 * so they are us, and (of course) cool.
	 */
	if (fDoneIt) {
		return 1;	
	}

	pwdSuper = getpwuid(0);	/* roots password always works */
	sprintf(acBuf, "Enter %s's password: ", pcGuilty);
	pcPass = getpass(acBuf);
	if (0 == CheckPass(pwdSuper, pcGuiltyPasswd, pcPass)) {
		fprintf(stderr, "Sorry.\n");
		exit(EPERM);
	}

	fDoneIt = 1;
	return 1;
}

typedef struct {
	char *pcGlob;
	int uid;
	int gid;
	char *pcOpts;
} OWNER;

/* an onwer record makes a dir if either thereis a -d in the opts	(ksb)
 * or if a training '/' is in the glob expression
 */
int
MakesDir(pON)
OWNER *pON;
{
	register char *pcDash, *pcChop;

	pcChop = strrchr(pON->pcGlob, '/');
	if ((char *)0 != pcChop && '\000' == pcChop[1]) {
		*pcChop = '\000';
		return 1;
	}
	for (pcDash = strchr(pON->pcOpts, '-'); (char *)0 != pcDash; pcDash = strchr(pcDash+1, '-')) {
		if ('-' == pcDash[1])	/* -- stops option processing */
			break;
		if ('d' == pcDash[1])
			return 1;
	}
	return 0;
}

static OWNER *pONlist = (OWNER *)0;
static int iOwners;
static char acOwnerPrefix[MAXPATHLEN], *pcOwnerPrefix;

/* test to see if user with uid in groups gidset is an owner of		(bj)
 * the entry i in pONlist.
 */
static int
IsOwner(i, uid, ngroups, gidset)
int i;
int uid;
int ngroups;
int *gidset;
{
	register int j, fUser, fGroup;

	fUser = (uid == pONlist[i].uid) || (-1 == pONlist[i].uid);
	if (-1 == pONlist[i].gid) {
		fGroup = 1;
	} else {
		fGroup = 0;
		for (j = 0; j < ngroups; ++j) {
			if (pONlist[i].gid == gidset[j]) {
				fGroup = 1;
				break;
			}
		}
	}
	return fUser && fGroup;
}

/* lookup (linear search, don't tell mom) a file in the cached		(bj)
 * .owners file and return a string with the install options, or
 * NULL if it doesn't exist or the user fails the security checks.
 */
char *
GetInstallOpts(pcFile, fDirOnly)
char *pcFile;
int fDirOnly;
{
	register int i;
	static int gidset[NGROUPS];  /* try NGROUPS_MAX under POSIX? */
	static int ngroups;
	static int *pGRFound = (void *)0;
	auto char acTest[MAXPATHLEN];

	if (0 == ngroups) {
		/* first invocation, find their groups
		 */
		pGRFound = (void *)gidset;
		ngroups = getgroups(NGROUPS, (void *)gidset);
		/* Any sanity check is a good one.  And if the system
		 * cannot find our groups mkcmd can.
		 */
		if (-1 == ngroups) {
			ngroups = util_getlogingr(username, & pGRFound);
			if (-1 == ngroups) {
				printf("%s: cannot determin group list\n", progname);
				return (char *)0;
			}
		}
		if (0 == ngroups) {
			printf("%s: You're not in any groups?!\n", progname);
			return (char *)0;
		}
	}

	/* checks password or uid/gid and only asks the first time
	 */
	if (! CheckAccess(getuid(), getgid(), pGRFound, ngroups)) {
		return (char *)0;
	}

	/* fDirOnly mean we have to have -d or a trailing '/' in the
	 * line for it to match -- ksb
	 */
	sprintf(acTest, "%s%s", pcOwnerPrefix, pcFile);
	for (i = 0; i < iOwners; ++i) {
		if (fDirOnly && !MakesDir(& pONlist[i]))
			continue;
		if (SamePath(pONlist[i].pcGlob, acTest))
			break;
	}
	if (i == iOwners) {
		fprintf(stderr, "%s: %s: Not a managed %s\n", progname, pcFile, fDirOnly ? "directory" : "file");
		return (char *)0;
	}

	/* We've found a match for the file, now check for
	 * permission.  The owner of the setuid installus can
	 * install anything, everyone else has to match the
	 * ownership specified in the .owners file.
	 */
	if (getuid() == geteuid() || IsOwner(i, getuid(), ngroups, pGRFound)) {
		return pONlist[i].pcOpts;
	}
	fprintf(stderr, "%s: %s: Not owner (see -Wv)\n", progname, pcFile);
	return (char *)0;
}

/* "parse" the .owners file into OWNER records for later lookup		(bj)
 * figure out what .owners file to use based on the destination path
 * return boolean success.
 */
int
ParseOwnersFor(pcDest, pstOwners, pcRemember)
char *pcDest, *pcRemember;
struct stat *pstOwners;
{
	register int iPathLen, fdOwners, i, iAlloc, fHash, iLineno;
	register char *pcBuf, *pcTemp;
	register struct passwd *passwd;
	register struct group *group;
	auto char acPath[MAXPATHLEN+4];

	if ('\000' == pcDest[0]) {
		pcDest = ".";
	}

	if (-1 == chdir(pcDest)) {
		fprintf(stderr, "%s: chdir: %s: %s\n", progname, pcDest, strerror(errno));
		return 0;
	}
	/* get the canonical name of the directory we're in.  as we look
	 * in parent directories for the owners file, we'll move
	 * pcOwnerPrefix back so it includes more of the path name to
	 * prepend when testing globs.
	 */
	if ((char *)0 == getwd(acOwnerPrefix)) {
		fprintf(stderr, "%s: getwd: %s\n", progname, strerror(errno));
		return 0;
	}
	pcOwnerPrefix = acOwnerPrefix + strlen(acOwnerPrefix);
	if ('/' != pcOwnerPrefix[-1]) {
		*pcOwnerPrefix++ = '/';
		*pcOwnerPrefix = '\000';
	}

	acPath[0] = '\000';
	iPathLen = 0;
	while(1) {
		strcpy(acPath+iPathLen, acOwners);
		if (-1 != (fdOwners = open(acPath, O_RDONLY, 0))) {
			break;
		}
		strcpy(acPath + iPathLen, "../");
		if ((iPathLen += 3) > MAXPATHLEN) {
			fprintf(stderr, "%s: Path too long while searching for owners file\n", progname);
			return 0;
		}
		/* move ptr back to include more of the pathname
		 */
		if ((pcOwnerPrefix -= 2) < acOwnerPrefix) {
			fprintf(stderr, "%s: %s: No owners file found\n", progname, pcDest);
			return 0;
		}
		while ('/' != pcOwnerPrefix[-1])
			--pcOwnerPrefix;
	}

	if (-1 == fstat(fdOwners, pstOwners)) {
		fprintf(stderr, "%s: fstat: %s: %s\n", progname, acPath, strerror(errno));
		return 0;
	}

	/* make sure that the files is owned by the effective uid
	 * of the process and has secure modes (root can own anyones
	 * owners file, BTW).
	 */
	if (0 != (pstOwners->st_mode & 0022) || !(geteuid() == pstOwners->st_uid || 0 == pstOwners->st_uid)) {
		close(fdOwners);
		fprintf(stderr, "%s: %s: File is insecure\n", progname, acPath);
		return 0;
	}
	if (pstOwners->st_mode & 04000) {
		iOwnerFileUid = pstOwners->st_uid;
	} else {
		iOwnerFileUid = -1;
	}
	if (pstOwners->st_mode & 02000) {
		iOwnerFileGid = pstOwners->st_gid;
	} else {
		iOwnerFileGid = -1;
	}

	pcBuf = (char *)malloc((pstOwners->st_size|15)+1);
	if (pstOwners->st_size != read(fdOwners, pcBuf, pstOwners->st_size)) {
		close(fdOwners);
		fprintf(stderr, "%s: read: %s: %s\n", progname, acPath, strerror(errno));
		return 0;
	}
	close(fdOwners);

	/* let's ensure it ends with a newline so it's an
	 * invariant below for parsing
	 */
	if (0 != pstOwners->st_size && '\n' != pcBuf[pstOwners->st_size-1]) {
		pcBuf[pstOwners->st_size++] = '\n';
	}
	pcBuf[pstOwners->st_size] = '\000';

	/* first count newlines and allocate that many OWNER structures.
	 * it's cheaper than realloc()'ing "cleverly"
	 */
	for (iAlloc = i = 0; '\000' != pcBuf[i]; ++i) {
		if ('\n' == pcBuf[i])
			++iAlloc;
	}
	if ((OWNER *)0 != pONlist) {
		free(pONlist);
	}
	iOwners = 0;
	pONlist = (OWNER *) calloc(iAlloc, sizeof(OWNER));

	/* zip through the big buf extracting the essence of life
	 */
	for (iLineno = 0; '\000' != *pcBuf; ++iLineno) {
		/* we should be at the beginning of a line here,
		 * a good time to notice comments
		 */
		if ('#' == *pcBuf) {
			while ('\n' != *pcBuf)
				++pcBuf;
			++pcBuf;
			continue;
		}
		/* skip leading whitespace, check for blank lines
		 */
		while (isspace(*pcBuf) && '\n' != *pcBuf)
			++pcBuf;
		if ('\n' == *pcBuf) {
			++pcBuf;
			continue;
		}

		/* we should point at the beginning of a glob
		 * expression now.  store it, find the end of it,
		 * zorch(tm) the first whitespace after it, then
		 * skip any extra whitespace to find the next thing...
		 */
		pONlist[iOwners].pcGlob = pcBuf;
		while (! isspace(*pcBuf))
			++pcBuf;
		if ('\n' == *pcBuf)
			break;
		*(pcBuf++) = '\000';  /* Zorch (tm) */
		while (isspace(*pcBuf) && '\n' != *pcBuf)
			++pcBuf;
		if ('\n' == *pcBuf)
			break;

		/* we should point to user.group now.  convert them
		 * to numeric for cheaper compares later.
		 */
		pcTemp = pcBuf;
		while ('\n' != *pcBuf && '.' != *pcBuf)
			++pcBuf;
		if ('\n' == *pcBuf)
			break;
		*(pcBuf++) = '\000';

		/* username is in pcTemp
		 */
		if (0 == strcmp("*", pcTemp)) {
			pONlist[iOwners].uid = -1;
		} else if ((struct passwd *)0 != (passwd = getpwnam(pcTemp))) {
			pONlist[iOwners].uid = passwd->pw_uid;
		} else if (alldigits(pcTemp)) {
			pONlist[iOwners].uid = atoi(pcTemp);
		} else {
			fprintf(stderr, "%s: %s: Unknown user\n", progname, pcTemp);
			break;
		}

		pcTemp = pcBuf;
		while (! isspace(*pcBuf))
			++pcBuf;
		if ('\n' == *pcBuf)
			break;
		*(pcBuf++) = '\000';
		/* group name in pcTemp
		 */
		if (0 == strcmp("*", pcTemp)) {
			pONlist[iOwners].gid = -1;
		} else if ((struct group *)0 != (group = getgrnam(pcTemp))) {
			pONlist[iOwners].gid = group->gr_gid;
		} else if (alldigits(pcTemp)) {
			pONlist[iOwners].gid = atoi(pcTemp);
		} else {
			fprintf(stderr, "%s: %s: Unknown group\n", progname, pcTemp);
			break;
		}

		/* skip more whitespace
		 */
		while (isspace(*pcBuf) && '\n' != *pcBuf)
			++pcBuf;
		if ('\n' == *pcBuf)
			break;

		/* and at long last, the Install options for this file
		 */
		pONlist[iOwners].pcOpts = pcBuf;
		for (fHash = 0; '\n' != *pcBuf; ++pcBuf) {
			if ('#' == *pcBuf) {
				fHash = 1;
			}
			if (fHash) {
				*pcBuf = '\000';
			}
		}
		*(pcBuf++) = '\000';
		++iOwners;
	}

	/* at this point we should be at the end of the buffer.
	 * if we're not, then we aborted parsing because of an error.
	 */
	if ('\000' != *pcBuf) {
		fprintf(stderr, "%s: %s(%d): Malformed file entry\n", progname, acPath, iLineno);
		return 0;
	}
	if ((char *)0 != pcRemember) {
		(void)strncpy(pcRemember, acPath, MAXPATHLEN+1);
	}
	return 1;
}

/* Tell us what files pcUser (defaults to getuid()) owns (in pcDir,	(bj)
 * if supplied, else everywhere).  When fExclusive, show only exact
 * matches.  The "everywhere" clause proved too much for me.
 * We might search the local filesystems for .owners files... nah.
 */
void
ShowOwned(pcUser, pcDir, fExclusive)
	char *pcUser, *pcDir;
	int fExclusive;
{
	register int uid, i;
	register char *pcTail, *pcGroup;
	register struct passwd *passwd;
	register struct group *grp;
	register char *pcSep;
	auto gid_t *pGRFound;
	auto gid_t gidset[NGROUPS];  /* try NGROUPS_MAX under POSIX? */
	auto int ngroups;
	auto struct stat stOwners;
	auto char acControl[MAXPATHLEN+4];
	static char acAnyUser[] = "*";

	if ((char *)0 == pcDir) {
		printf("%s: sorry, I'm not smart enough to work without a dir yet\n", progname);
		return;
	}
	/* let "-L -- -- /usr/local/bin/.owners" mean "... /usr/local/bin"
	 */
	if ((char *)0 != (pcTail = strrchr(pcDir, '/')) && 0 == strcmp(acOwners, pcTail+1)) {
		*pcTail = '\000';
	}
	if (! ParseOwnersFor(pcDir, & stOwners, acControl)) {
		return;
	}

	/* only let the program owner list specific users, peons
	 * can only list for themselves, unless the file is world read.
	 */
	if ((char *)0 == pcUser) {
		pcGroup = (char *)0;
	} else if ((char *)0 != (pcGroup = strchr(pcUser, ':')) || (char *)0 != (pcGroup = strchr(pcUser, '.'))) {
		*pcGroup++ = '\000';
	}
	if ((char *)0 != pcGroup && '\000' == *pcGroup) {
		pcGroup = (char *)0;
	}
	if ((char *)0 != pcUser && '\000' == *pcUser) {
		pcUser = (char *)0;
	}

	/* who are we asking about?
	 */
	if ((char *)0 == pcUser) {
		uid = getuid();
		passwd = getpwuid(uid);
		if ((struct passwd *)0 != passwd) {
			pcUser = passwd->pw_name;
		} else {
			pcUser = calloc(32, sizeof(char));
			sprintf(pcUser, "%d", uid);
		}
		uid = getuid();
	} else if ('*' == pcUser[0] && '\000' == pcUser[1]) {
		passwd = (struct passwd *)0;
		pcUser = acAnyUser;
		uid = -1;
	} else {
		if ((struct passwd *)0 == (passwd = getpwnam(pcUser))) {
			passwd = getpwuid(atoi(pcUser));
		}
		if ((struct passwd *)0 == passwd) {
			fprintf(stderr, "%s: getpwent: %s: unknown user\n", progname, pcUser);
			return;
		}
		uid = passwd->pw_uid;
	}

	/* And what group are the in (not really, but if they were...)
	 * Since it takes 800 lines of code to do getgroups() for
	 * a user other than ourselves, we'll have to punt for now
	 * and just use login group, which is so incredibly cheap
	 * to get (1 line of code). XXX			-- bj
	 * As a work around maybe we could take "login:group,group" -- ksb
	 */
	ngroups = 1;
	gidset[0] = -1;
	pGRFound = gidset;
	if ((char *)0 == pcGroup) {
		if ((struct passwd *)0 != passwd) {
			ngroups = util_getlogingr(passwd->pw_name, & pGRFound);
			if (-1 == ngroups) {
				fprintf(stderr, "%s: %s: cannot determin groups\n", progname, passwd->pw_name);
				return;
			}
			if (0 == ngroups) {
				fprintf(stderr, "%s: %s: you're not in any groups?!\n", progname, passwd->pw_name);
				/* OK, we'll not check a group */
			}
		}
	} else if ('*' == pcGroup[0] && '\000' == pcGroup[1]) {
		gidset[0] = -1;
	} else {
		if ((struct group *)0 == (grp = getgrnam(pcGroup))) {
			grp = getgrgid(atoi(pcGroup));
		}
		if ((struct group *)0 == grp) {
			fprintf(stderr, "%s: getgrent: %s: %s\n", progname, pcGroup, strerror(errno));
			exit(1);
		}
		gidset[0] = grp->gr_gid;
	}

	if (fVerbose) {
		printf("%s: list for", progname);
		if (-1 == uid) {
			printf(" any login");
		} else {
			printf(" %s [%d]", pcUser, uid);
		}
		printf(", in");
		if (1 == ngroups && -1 == pGRFound[0]) {
			printf(" any group");
		} else {
			pcSep = (1 == ngroups) ? "" : " any of:";
			for (i = 0; i < ngroups; ++i ) {
				grp = getgrgid(pGRFound[i]);
				if ((struct group *)0 != grp) {
					printf("%s %s [%d]", pcSep, grp->gr_name, pGRFound[i]);
				} else {
					printf("%s %d", pcSep, pGRFound[i]);
				}
				pcSep = ",";
			}
		}
		printf(" from %s\n", acControl);
	}
	if (getuid() == uid || geteuid() == getuid()) {
		/* we are the Dude, or not setuid */
	} else if (0 != (0040 & stOwners.st_mode) && getgid() == stOwners.st_gid) {
		/* we are in the group of the file and it may read */
	} else if (0 != (0004 & stOwners.st_mode)) {
		/* owners is world read */
	} else {
		fprintf(stderr, "%s: list %s: Permission denied\n", progname, pcUser);
		exit(13);
	}

	pcOwnerPrefix[0] = '\000';
	for (i = 0; i < iOwners; ++i) {
		/* if we require exactly one maintainer (fExclusive)
		 * and this has '*' as the owner, we can ignore it.
		 */
		if (fExclusive && -1 == pONlist[i].uid) {
			continue;
		}
		/* otherwise, when they control it output it
		 */
		if (IsOwner(i, uid, ngroups, pGRFound)) {
			printf("%s%s\n", acOwnerPrefix, pONlist[i].pcGlob);
		}
	}
}

/* show who owns a file							(bj)
 */
void
ShowOwner(pcPath)
char *pcPath;
{
	register struct group *group;
	register struct passwd *passwd;
	register int i;
	register int fDirOnly;
	auto char *pcFile;
	auto char acTest[MAXPATHLEN+4], acControl[MAXPATHLEN+4];
	auto struct stat stOwners;

	/* read training '/' as a -d flag but do not eat "/" (the root dir)
	 */
	fDirOnly = fMustDir;
	if ((char *)0 != (pcFile = strrchr(pcPath, '/')) && '\000' == pcFile[1]) {
		fDirOnly = 1;
		if (pcFile != pcPath) {
			*pcFile = '\000';
		}
	}
	/* We should stat(2) the file to see if it _is_ a directory
	 * to set fDirOnly if it is not set.  This makes me happy. --ksb
	 */
	if (!fDirOnly && -1 != stat(pcPath, & stOwners) && S_IFDIR == (stOwners.st_mode & S_IFMT)) {
		fDirOnly = 1;
	}

	/* XXX we rescan the file every time even if they're all in
	 * the same dir.  bletch!
	 */
	if ((char *)0 == (pcFile = strrchr(pcPath, '/'))) {
		pcFile = pcPath;
		if (!ParseOwnersFor(".", & stOwners, acControl)) {
			return;
		}
	} else {
		*pcFile = '\000';
		if (!ParseOwnersFor(pcFile == pcPath ? "/" : pcPath, & stOwners, acControl)) {
			*pcFile = '/';
			return;
		}
		*pcFile++ = '/';
	}

	sprintf(acTest, "%s%s", pcOwnerPrefix, pcFile);
	for (i = 0; i < iOwners; ++i) {
		if (fDirOnly && !MakesDir(& pONlist[i]))
			continue;
		if (SamePath(pONlist[i].pcGlob, acTest))
			break;
	}
	if (i == iOwners) {
		fprintf(stderr, "%s: %s: Not a managed %s [in %s]\n", progname, pcFile, fDirOnly ? "directory" : "file", acControl);
		return;
	}
	printf("%s%s: ", pcPath, "/"+(!fDirOnly));
	if (-1 != iOwnerFileGid || -1 != iOwnerFileUid) {
		register int iOr;

		iOr = 0;
		if (fVerbose) {
			printf("[%s requires no password for", acControl);
		} else {
			printf("[no password for");
		}
		if (-1 != iOwnerFileUid) {
			if ((struct passwd *)0 == (passwd = getpwuid(iOwnerFileUid))) {
				printf(" uid %d", iOwnerFileUid);
			} else {
				printf(" login %s", passwd->pw_name);
			}
			iOr = 1;
		}
		if (-1 != iOwnerFileGid) {
			if (0 != iOr) {
				printf(", or");
			}
			if ((struct group *)0 == (group = getgrgid(iOwnerFileGid))) {
				printf(" gid %d", iOwnerFileGid);
			} else {
				printf(" group %s", group->gr_name);
			}
		}
		printf("] ");
	} else if (fVerbose) {
		printf("[reading %s] ", acControl);
	}
	if (-1 == pONlist[i].uid && -1 == pONlist[i].gid) {
		printf("anyone\n");
		return;
	}
	if (-1 == pONlist[i].uid) {
		if ((struct group *)0 == (group = getgrgid(pONlist[i].gid))) {
			printf("anyone in gid %d\n", pONlist[i].gid);
		} else {
			printf("anyone in group %s\n", group->gr_name);
		}
		return;
	}
	if (-1 == pONlist[i].gid) {
		if ((struct passwd *)0 == (passwd = getpwuid(pONlist[i].uid))) {
			printf("uid %d\n", pONlist[i].uid);
		} else {
			printf("login %s\n", passwd->pw_name);
		}
		return;
	}
	if ((struct passwd *)0 == (passwd = getpwuid(pONlist[i].uid))) {
		printf("only %d.", pONlist[i].uid);
	} else {
		printf("only %s.", passwd->pw_name);
	}
	if ((struct group *)0 == (group = getgrgid(pONlist[i].gid))) {
		printf("%d\n", pONlist[i].gid);
	} else {
		printf("%s\n", group->gr_name);
	}
}

/* is the string all digits?						(bj)
 */
static int
alldigits(pc)
char *pc;
{
	for (; '\000' != *pc; ++pc)
		if (!isdigit(*pc))
			return 0;
	return 1;
}
