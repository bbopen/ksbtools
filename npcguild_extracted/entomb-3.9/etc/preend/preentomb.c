/* $Id: preentomb.c,v 3.12 2000/07/31 15:07:35 ksb Exp $
 * clean out a tomb and all the user crypts				(ksb)
 * this file sets all the policy for keeping things in the tombs
 */
#ifndef lint
static char rcsid[] =
	"$Id: preentomb.c,v 3.12 2000/07/31 15:07:35 ksb Exp $";
#endif

#include <stdio.h>
#include <ctype.h>
#include <syslog.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "libtomb.h"
#include "main.h"
#include "preentomb.h"

/* #include <fstab.h> */

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if HAS_NDIR
#include <ndir.h>
#else
#if HAS_DIRENT
#include <dirent.h>
#else
#include <sys/dir.h>
#endif
#endif

#if USE_STATFS
#if NEXT2 || LINUX
#include <sys/vfs.h>
#else
#if FREEBSD || NETBSD || BSDI
#include <sys/mount.h>
#else
#include <sys/statfs.h>
#endif
#endif

#else	/* guess */

#if SUN3 || SUN4
#include <ufs/fs.h>
#else
#if SUN5
#include <sys/vnode.h>
#include <sys/fs/ufs_fs.h>
#include <sys/fs/ufs_inode.h>
#else
#include <sys/fs.h>
#endif	/* solaris */
#endif	/* sunos */
#endif

#if !defined(DEV_BSIZE)
#define DEV_BSIZE       512
#endif

uid_t uidCharon = -1;
gid_t gidCharon = -1;

extern long atol();
extern int errno;

#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif


/* read the super block from a fs					(mjb)
 */
static int
bread(fi, bno, buf, cnt)
int fi;
daddr_t bno;
char *buf;
int cnt;
{
	extern off_t lseek();
	register int n;

	(void) lseek(fi, (off_t)(bno * DEV_BSIZE), 0);
	while (0 != cnt && cnt != (n = read(fi, buf, cnt))) {
		if (n > 0) {
			buf += n;
			cnt -= n;
			continue;
		}
		(void)fprintf(stderr, "preend: read: %s\nbno = %ld, count = %d; errno = %d\n\n", strerror(errno), bno, n, errno);
		return -1;
	}
	return 0;
}

/*
 * returns a percent (eg. 95) indicating how full a file system is	(mjb)
 */
#if USE_STATFS
int
HowFull1(pcFile)
char
	*pcFile;		/* a file on the mounted device		*/
{
	long lTotal, lAvail, lFree, lUsed, lInodes;
	int disk_prcnt, ino_prcnt;

	auto struct statfs sfBuf;

	if (-1 == statfs(pcFile, & sfBuf))
		return -1;

	lTotal = sfBuf.f_blocks;
	lAvail = lTotal - (sfBuf.f_bfree-sfBuf.f_bavail);
	lFree = sfBuf.f_bavail;
	lUsed = lTotal - sfBuf.f_bfree;
	disk_prcnt = (lAvail == 0) ? 100 :
			(double) lUsed / (double) lAvail * 100.0;
	lUsed = sfBuf.f_files - sfBuf.f_ffree;
	ino_prcnt = 100.0 * lUsed/sfBuf.f_files;

	if (disk_prcnt > ino_prcnt) {
		return disk_prcnt;
	}
	return ino_prcnt;
}
#else
int
HowFull2(fi)
int
	fi;			/* an open fd on the device node	*/
{
	long lTotal, lAvail, lFree, lUsed, lInodes;
	int disk_prcnt, ino_prcnt;
	static union {
		struct fs iu_fs;
		char dummy[SBSIZE];
	} sb;
#define sblock (sb.iu_fs)

	if (-1 == fi) {	/* we do not know, missing device node, guess */
		return -1;
	}

	/* read the superblock
	 */
	if (-1 == bread(fi, SBLOCK, (char *)&sblock, sizeof(sb))) {
		return -1;
	}

	lTotal = sblock.fs_dsize;
	lFree = sblock.fs_cstotal.cs_nbfree * sblock.fs_frag +
		sblock.fs_cstotal.cs_nffree;
	lUsed = lTotal - lFree;
	lAvail = lTotal * (100 - sblock.fs_minfree) / 100;

	disk_prcnt = (lAvail == 0) ? 100 :
			(double) lUsed / (double) lAvail * 100.0;
	lInodes = sblock.fs_ncg * sblock.fs_ipg;
	lUsed = lInodes - sblock.fs_cstotal.cs_nifree;
	ino_prcnt = (lInodes == 0) ? 100 :
			(double) lUsed / (double) lInodes * 100.0;
#undef sblock

	if (disk_prcnt > ino_prcnt) {
		return disk_prcnt;
	}
	return ino_prcnt;
}
#endif

/* Decide whether a file should be removed yet, according to		(ksb)
 * its name, its size, and the fullness of its filesystem.
 * Returns 1 (keep) or 0 (for dink).
 */
static int
IsYoung(lSize, tAge, iFullness)
unsigned long lSize;		/* in bytes		*/
long tAge;			/* creation date	*/
int iFullness;			/* as a percent		*/
{
	long lifespan;

#ifdef DEBUG
	fprintf(stderr, "IsYoung(%ld, %ld, %d)\n", lSize, tTombed, iFullness);
#endif

	/* fs is a max, keep nothing
	 */
	if (iFullness > FULL_3) {
		return 0;
	}

	/* fs is close, keep everything a little while (about 0-15 minutes)
	 */
	if (iFullness > FULL_2) {
		if (tAge <= MINTIME)
			return 1;
		return 0;
	}

	/* When the file is really young keep is in any case now
	 */
	if (tAge <= MINTIME) {
		return 1;
	}

	/* When the file system is mostly used begin aging files,
	 * compute a `life span' (crypt span?) for the file base on
	 * it's size and stuff
	 */
	if (iFullness > FULL_1) {
		lifespan = TIME_2 - ((double) SECPERBYTE * (double) lSize);
		if (tAge > lifespan)
			return 0;
		return 1;
	}

	/* If files have been here for atleast a full backup (or something
	 * like that) then we dink'm even if the file system is almost
	 * empty {after all they were rm'd, right?)
	 */
	return tAge <= TIME_1;
}

/*
 * check the given file name to see if we should dink it,	(mjb&ksb)
 * based only on it's age and it's name.  So the fact that
 * worthless files only survive MINTIME is determined here.
 */
static int
IsWorthless(pchName)
char *pchName;
{
	register char *pcExt;

	/* is the file a file gen'd by the loader, os, or
	 * rpof, gprof, yacc, lex, etc.
	 */
	switch (*pchName) {
	case '#':
		return 1;
	case 'a':
		if (0 == strcmp(pchName, "a.out"))
			return 1;
		break;
	case 'c':
		if (0 == strcmp(pchName, "core"))
			return 1;
		break;
	case 'g':
		if (0 == strcmp(pchName, "gmon.out") || 0 == strcmp(pchName, "gmon.sum"))
			return 1;
		break;
	case 'l':
		if (0 == strcmp(pchName, "lex.yy.c"))
			return 1;
		break;
	case 'm':
		if (0 == strcmp(pchName, "mon.out") || 0 == strcmp(pchName, "mon.sum"))
			return 1;
		break;
	case 'y':
		if (0 == strcmp(pchName, "y.output") || 0 == strcmp(pchName, "y.tab.c"))
			return 1;
		break;
	case '.':
		if (0 == strcmp(pchName, ".xerrors"))
			return 1;
		break;
	default:
		break;
	}

	/* does the file have a vulgar extender?
	 */
	pcExt = strrchr(pchName, '.');
	if ((char *)0 != pcExt) {
		/*
		 * for the rest we're only concerned with the extension.
		 * filenames w/o extensions aren't worthless.
		 */
		switch (*++pcExt) {
		case 'a':		/* ar lib		*/
			if ('\000' == pcExt[1])
				return 1;
			break;
		case 'b':		/* Makefile.bak		*/
			if (0 == strcmp(pcExt, "bak"))
				return 1;
			break;
		case 'o':		/* cc, pc, f77		*/
			if ('\000' == pcExt[1])
				return 1;
			break;
		case 'd':		/* fortran debug	*/
			if (0 == strcmp(pcExt, "dbg"))
				return 1;
			break;
		case 's':		/* cc, pc, f77 -S	*/
			if ('\000' == pcExt[1])
				return 1;
			break;
		default:		/* don't know yet	*/
			break;
		}
	}

	/* is the file an emacs backup file
	 * (if so the user rm'd it which was the `second chance' we designed
	 * for already done for us)
	 */
	if ('~' == *(pchName + strlen(pchName) - 1)) {
		return 1;
	}

	/* must be a cool file, keep it
	 */
	return 0;
}

/* preen a user's tomb directory, like /usera/tomb/11239		(ksb)
 *
 * We re-check the file system's size after dinking some files,
 * because it might change our metric for the other files.  We return
 * 0 for OK, 1 for the crypt is bogus, 2 for give up on the fs.
 */
static int
PreenCrypt(pchPath, iHowFull)
char
	*pchPath;			/* e.g. "/usera/tomb/mjb"	*/
int
	iHowFull;			/* the fullness of this fs	*/
{
	register DIR *pD;		/* directory being preened	*/
	register struct DIRENT *pd;	/* current directory entry	*/
	register char *pch;		/* pointer into name		*/
	register char *pcTail;		/* the end of the crypt		*/
	register int iRet;		/* our return value		*/
	auto struct stat statb;		/* handy buffer for stat	*/
	auto long tombTime;		/* time entombed		*/
	struct timeval tp;		/* figure out what time it is	*/
	struct timezone tzp;		/* ... same			*/
	auto time_t tNow;		/* the current secs since epoch */

	if (NULL == (pD = opendir(pchPath))) {
		syslog(LOG_ERR, "preentomb: opendir: %s: %m", pchPath);
		return 1;
	}

	/* check our watch and disk usage meter
	 */
	if (-1 == gettimeofday(&tp, &tzp)) {
		syslog(LOG_ERR, "preentomb: gettimeofday: %m");
		return 1;
	}
	iRet = 0;
	tNow = tp.tv_sec;

	/* read the filenames out of the directory.  Filenames are of the
	 * format "filename@timestamp"
	 */
	pcTail = pchPath + strlen(pchPath);
	*pcTail++ = '/';
	while (NULL != (pd = readdir(pD))) {
		pch = pd->d_name;
		if ('.' == pch[0] && ('\000' == pch[1] ||
		    ('.' == pch[1] && '\000' == pch[2])))
			continue;
		(void)strcpy(pcTail, pch);
		if (stat(pchPath, & statb) < 0 ) {
			syslog(LOG_ERR, "stat %s: %m", pchPath);
			continue;
		}

		/* take the file from the user, so it will no longer
		 * count towards the user's quota.  If entomb is trusted
		 * to change the group files live for less time.
		 */
		if (gidCharon != statb.st_gid || uidCharon != statb.st_uid) {
			if (Debug) {
				printf("%s: chmod %04o %s\n", progname, (statb.st_mode & 0700) | 0440, pchPath);
				printf("%s: chown %d.%d %s\n", progname, uidCharon, gidCharon, pchPath);
			}
			if (fExec) {
				chmod(pchPath, (statb.st_mode & 0700) | 0440);
				chown(pchPath, uidCharon, gidCharon);
			}
			continue;
		}

		/* find the last @, and get the time that the file was
		 * entombed.
		 */
		pch = strrchr(pcTail, '@');
		if (NULL == pch) {
			continue;
		}
		if (0 == (tombTime = atol(pch+1))) {
			/* ZZZ LLL XXX ? How?  What does it mean?
			 * (it means that someone other than entomb put it here)
			 */
			continue;
		}
		if (!IsWorthless(pcTail) && IsYoung((unsigned long)statb.st_size, tNow-tombTime, iHowFull)) {
			continue;
		}

		/* we're going to dink this one
		 */
		if (Debug) {
			printf("%s: rm -f %s\n", progname, pchPath);
		}
		if (fExec && -1 == unlink(pchPath) && ENOENT != errno) {
			if (EROFS == errno) {
				iRet = 1;
				break;
			}
			syslog(LOG_ERR, "unlink: %s: %m", pchPath);
		}
	}
	*--pcTail = '\000';
	(void)closedir(pD);
	return iRet;
}

/* preen the tomb directory on a particular filesystem			(ksb)
 * we're given the filesystem name and path to the tomb.
 *  e.g:  preenfs("/dev/zd1a", "/usera/tomb")
 * We open the device so we can watch the changes we make,
 * and preen each crypt.
 * If the dir is "." we do a little savings in namei.
 */
int
PreenTomb(pchDev, pchTombDir)
	char *pchDev;			/* the raw disk device involved */
	char *pchTombDir;		/* the name of the tomb directory */
{
	register DIR *pD;		/* directory being preened	*/
	register struct DIRENT *pd;	/* current directory entry	*/
	register char *pcTail;		/* tail to copy crypt names to	*/
	register char *pch;		/* temp char pointer		*/
	register int iHowFull;		/* how full is the file system	*/
	auto struct stat statb;		/* handy buffer for stat	*/
	auto char sbPath[MAXPATHLEN + MAXNAMLEN + 2];
#if !USE_STATFS
	auto int fdDev;		/* open device			*/
#endif


	/* Make the tomb name, open the device for %utilization info
	 * open the tomb dir to scan for crypts.
	 */
	(void)strcpy(sbPath, pchTombDir);

#if !USE_STATFS
	if (-1 == (fdDev = open(pchDev, O_RDONLY, 0)) && ENOENT != errno) {
		fprintf(stderr, "%s: open: %s: %s\n", progname, pchDev, strerror(errno));
	}
#endif

	if (NULL == (pD = opendir(pchTombDir))) {
		syslog(LOG_ERR, "opendir: %s: %m", sbPath);
#if !USE_STATFS
		(void)close(fdDev);
#endif
		return -1;
	}

	/* we read in a list of files, which we expect to be subdirectories,
	 * each a uid, full of files entombed by that user.
	 */
	if (0 == strcmp(".", sbPath)) {
		pcTail = sbPath;
	} else {
		pcTail = sbPath + strlen(sbPath);
		*pcTail++ = '/';
	}
	while (NULL != (pd = readdir(pD))) {
		/* check an build the crypt name
		 * (skip `.' and `..' because we know these are not crypts)
		 * XXX maybe all nodes that start with '.' should be exempt
		 */
		pch = pd->d_name;
		if ('.' == pch[0] && ('\000' == pch[1] ||
		    ('.' == pch[1] && '\000' == pch[2])))
			continue;
		(void)strcpy(pcTail, pch);

		/* look to see if this is really a crypt, or maybe a
		 * .plan or .login for charon...
		 * Of course charon can't have a bin or anything  :-(
		 */
		if (-1 == stat(sbPath, & statb)) {
			if (ENOENT != errno) {
				syslog(LOG_ERR, "preentomb: stat: %s: %m", sbPath);
			}
			continue;
		}
		if (S_IFDIR != (statb.st_mode&S_IFMT)) {
			continue;
		}
#if USE_STATFS
		iHowFull = HowFull1(pchTombDir);
#else
		iHowFull = HowFull2(fdDev);
#endif
		/* we are on a readonly filesystem
		 */
		if (2 == PreenCrypt(sbPath, iHowFull)) {
			syslog(LOG_ERR, "preentomb: %s: %s", sbPath, strerror(EROFS));
			break;
		}

		if (!fEmpty) {		/* keep empty crypts		*/
			continue;
		}
		/* NB: We used to dink (unlink) empty crypts here.	(ksb)
		 *  An at-boot or rc.local find should do it; so we don't
		 *  anymore.  Also the moduser directive to rm a login
		 *  could remove the tomb for the uid.
		 */
		if (Debug) {
			printf("%s: rmdir %s\n", progname, sbPath);
		}
		if (fExec && -1 == rmdir(sbPath) && EROFS == errno) {
			syslog(LOG_ERR, "preentomb: %s: %s", sbPath, strerror(EROFS));
			break;
		}
	}
	if (sbPath != pcTail) {
		*--pcTail = '\000';
	}
	(void)closedir(pD);
#if !USE_STATFS
	if (-1 != fdDev) {
		(void)close(fdDev);
	}
#endif
	return 0;
}
