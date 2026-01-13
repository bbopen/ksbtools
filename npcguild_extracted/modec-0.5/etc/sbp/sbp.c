/* $Id: sbp.c,v 1.17 2000/05/08 21:12:52 ksb Exp $
 * Actually do the sync operations.
 * We can use any method in the table, see aSMTable below
 */
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include "machine.h"
#include "main.h"
#include "read.h"
#include "sbp.h"


/* return 1 if this exists and is a directory				(ksb)
 */
int
IsDir(pcNode)
char *pcNode;
{
	auto struct stat stNode;

	if (-1 == stat(pcNode, & stNode)) {
		if (ENOENT != errno) {
			fprintf(stderr, "%s: stat: %s: %s\n", progname, pcNode, strerror(errno));
		}
		return 0;
	}
	if (S_IFDIR == (stNode.st_mode & S_IFMT)) {
		return 1;
	}
	return 0;
}


/* Embedded newfs options on the end of the line have			(ksb)
 * to be separated from the line by a octothorpe and
 * start with a "-[A-Za-z]" after some white space.
 *	/dev/device /home ufs 0 2  # -i 16384 -c 24
 */
static char *
NewfsOpts(pcRest)
char *pcRest;
{
	register char *pcHash;

	pcHash = strchr(pcRest, '#');
	if ((char *)0 == pcHash) {
		return (char *)0;
	}
	do {
		++pcHash;
	} while (isspace(*pcHash));
	if ('-' != pcHash[0] || !isalpha(pcHash[1])) {
		return (char *)0;
	}
	return pcHash;
}

/* At 90% full of data we'd like the filesystem to have about 60% of	(ksb)
 * the inodes allocated.  These are tuneable parameters as well (-D/-I).
 *
 * The closer to 90% (or above) the fs is now the tighter we can
 * make the estimate of bytes/inode to get there.  For lower targets
 * we hedge to the "more inodes" side (because running out of inodes
 * with space left is silly).	-- ksb
 *
 * On the other side allocating more than 1 inode per frag is useless.
 * So we need to be sure that bytes-per-inode is >= frag-size.
 */
static int
MakeOpts(pcWhere, pcOpts)
char *pcWhere, *pcOpts;
{
	auto STATFS_BUF SFOn;
	register long lFused, lBused, lBlocks;
	register long lMinFree, lBytePerBlock;
	register double dActComp, dFiles;
	register double dBytePerFile, dBytePerInode, dFudge, dTemp;
	register int iMinFree;

	if (-1 == STATFS_CALL(pcWhere, & SFOn)) {
		return 0;
	}

	/* reproduce the df parameters I think in...
	 */
	lBused = SFOn.f_blocks - SFOn.f_bfree;
	lFused = SFOn.f_files - SFOn.f_ffree;
	dActComp = (100.0*(double)(lBused + SFOn.f_bavail)/(double)SFOn.f_blocks);
	lMinFree = (long) (100.50 - dActComp);
#if HAVE_STATVFS
	lBytePerBlock = SFOn.f_frsize;
#else
	lBytePerBlock = SFOn.f_bsize;
#endif
	if (fInsane) {
		printf("For %s:\n", pcWhere);
		printf(" %ld + %ld = %ld (used +  free = blocks)\n", lBused, SFOn.f_bfree, SFOn.f_blocks);
		printf(" %ld (avail blocks of %ld bytes)\n", SFOn.f_bavail, lBytePerBlock);
		printf(" %ld + %ld = %ld (used + free = files)\n", lFused, SFOn.f_ffree, SFOn.f_files);

		printf(" %2.4lf active, with minfree %ld\n", dActComp, lMinFree);
	}

	/* OK, we know everything, now compute the present bytes/inode
	 */
	dBytePerFile = (double)(lBused * lBytePerBlock) / (double)lFused;
	dFiles = ((double)SFOn.f_blocks * dBlockFull/100.0 * dActComp/100.0 * (double)lFused)/
	    ((double)lBused  * dFileFull/100.0);
	dBytePerInode = (double)SFOn.f_blocks * (double)lBytePerBlock * dActComp/100.0 / dFiles;

	/* The closer to the target % we are now the better the
	 * guess.  If we are way low round the bytes/inode down
	 * a "little". -- ksb
	 */
	dFudge = (double)SFOn.f_bavail/(double)SFOn.f_blocks;
	if (dFudge < 0.015021964) {
		dFudge = 0.0;
	} else if (dFudge > 0.18) {
		dFudge -= 0.06221965;
	}
	/* for sbp we know fudge can be a lot less
	 */
	dFudge /= 20.0;
	/* ZZZ the lower bound on bytes/inode is hard to figure
	 * and we are wrong here.
	 */
	lBlocks = (dBytePerInode+0.99990)/512.0 /* - dFudge */;
	if (dBytePerInode > MAX_BPI) {
		sprintf(pcOpts, "-i %ld", MAX_BPI);
	} else if (dBytePerInode < lBytePerBlock) {
		sprintf(pcOpts, "-i %ld", lBytePerBlock);
	} else {
		sprintf(pcOpts, "-i %ld", 512L * lBlocks);
	}

	/* Try to keep -b and -f options if we can, should be an option
	 * to us to guess as new ones, or keep these.
	 * we keep -m as well.  All this has "poor control".
	 */
#if HAVE_STATVFS
	if (0 != SFOn.f_bsize && 8192 != SFOn.f_bsize) {
		sprintf(pcOpts+strlen(pcOpts), " -b %d", SFOn.f_bsize);
	}
	if (0 != SFOn.f_frsize) {
		sprintf(pcOpts+strlen(pcOpts), " -f %d", SFOn.f_frsize);
	}
#else
	if (0 != SFOn.f_bsize) {
		sprintf(pcOpts+strlen(pcOpts), " -b %d", SFOn.f_bsize);
	}
#endif
	/* We should keep softupdates as well sblock.fs_flags |= FS_DOSOFTDEP;
	 * To do that we must read the superblock -- I don't have a clean
	 * way to do that (today, yet). -- ksb			ZZZ LLL
	 */
	/* We also can't pass -nenable to newfs, we must call tunefs
	 * How? XXX
	 */

	/* If the partition is mostly full we might reduce minfree here?
	 * When the source is >98.5% full and minfree >1% we should reduce
	 * minfree by 1%.  LLL  Can the default minfree change from "10"?
	 */
	iMinFree = (int)(lMinFree + 0.10);
	if (((double)SFOn.f_bavail)/SFOn.f_blocks < 1.5 && iMinFree > 1) {
		--iMinFree;
	}
	if (10 != iMinFree) {
		sprintf(pcOpts+strlen(pcOpts), " -m %d", iMinFree);
	}

	if (fInsane) {
		printf("Computed:\n");
		printf(" %lf bytes/file presently\n", dBytePerFile);
		printf(" We'd make that %6.0lf files at %7.0lf bytes/inode\n", dFiles, dBytePerInode);
		printf(" ...and that gets a fudge factor of %lf\n", dFudge);
		printf(" ...with %ld bytes/frag\n", lBytePerBlock);
		printf("Suggested newfs option \"%s\"\n", pcOpts);
	}
	return 1;
}

/* map /dev/dsk/c_t_d_s_ -> /dev/rdsk					(ksb)
 * also has to work for /dev/md/dsk/d72 and the like
 */
static void
MapRaw(pcOut, pcBlock)
char *pcOut, *pcBlock;
{
	register char *pcTok;
	register int iLen;
	if ((char *)0 != (pcTok = strstr(pcBlock, "/dsk/"))) {
		iLen = pcTok - pcBlock;
		sprintf(pcOut, "%*.*s/rdsk/%s", iLen, iLen, pcBlock, pcTok+5);
		return;
	}
	if ((char *)0 == (pcTok = strrchr(pcBlock, '/'))) {
		sprintf(pcOut, "/dev/rdsk/%s", pcBlock);
		return;
	}
	sprintf(pcOut, "/dev/r%s", pcBlock+5);
}

/* remember the method params the Custoerm suggested for "-opts"	(ksb)
 *  rsync:--verbose -> "--verbose" in %P
 */
static char *pcMethParams = (char *)0;
static void
ParamDash(pcOpts)
char *pcOpts;
{
	pcMethParams = ((char *)0 != pcOpts && '\000' == pcOpts[0]) ? (char *)0 : pcOpts;
}

/* for "dump" we have to remember the keys, then the params		(ksb)
 *	"dump:ub 126" -> "ub" in %K, "126" in %P
 * we take a const char array for inits
 */
static char *pcMethKeys = "";
static void
ParamDump(pcKey)
char *pcKey;
{
	if ((char *)0 == pcKey) {
		return;
	}
	pcMethKeys = strdup(pcKey);
	pcKey = pcMethKeys;
	while ('\000' != *pcKey && !isspace(*pcKey)) {
		++pcKey;
	}
	if (isspace(*pcKey)) {
		*pcKey = '\000';
		ParamDash(pcKey+1);
	}
}

/* expand the command from a few escapes we find handy			(ksb)
 *	%X...	our mirror (so %Xd is our mirror's devname)
 *	%d	devname
 *	%i	newfs tune options (-i, -c, -a, ...) from line, or
 *		under a command line a good guess
 *	%m	mount point
 *	%r	raw devname
 *
 *	%a	the string "-o async" or nothing (see -a)
 *	%f	the fstab file we read for this run ("./thing")
 *	%F	the fstab file we know the system uses ("/etc/vfstab")
 *	%D	acDumpPath[] = "/sbin/dump";
 *	%R	acRestorePath[] = "/sbin/restore";
 *	%I	acInstallPath[] = "/usr/local/bin/install";
 *
 *	%K	the keys part of the method params, if it has one (viz. dump)
 *	%P	any method suggested parameters
 */
static void
Expand(pcOut, pcIn, pBPSource)
char *pcOut, *pcIn;
SBP_ENTRY *pBPSource;
{
	register SBP_ENTRY *pBP;
	register char *pcTemp;
	auto struct stat stDev;

	while ('\000' != *pcIn) {
		if ('%' != *pcIn) {
			*pcOut++ = *pcIn++;
			continue;
		}
		pBP = pBPSource;
		*pcOut = '\000';	/* in cases of empty escapes */
		for (++pcIn;;) { switch (*pcIn++) {
			static char acAsync[] = "async";
			register char *pcWhite, *pcRaw, cKeep;
		case 'a':
			if (!fAsync && (char *)0 == strstr(pBP->pcrest, acAsync) && (char *)0 == strstr(pBP->pBPswap->pcrest, acAsync)) {
				break;
			}
			sprintf(pcOut, "-o %s", acAsync);
			break;
		case 'f':
			(void)strcpy(pcOut, pcFstab);
			break;
		case 'F':
			(void)strcpy(pcOut, acFstabPath);
			break;
		case 'D':
			(void)strcpy(pcOut, acDumpPath);
			break;
		case 'R':
			(void)strcpy(pcOut, acRestorePath);
			break;
		case 'I':
			(void)strcpy(pcOut, acInstallPath);
			break;
		case 'X':
			pBP = pBP->pBPswap;
			continue;
		case 'd':
			if (6 == iTableType) {
				(void)strcpy(pcOut, pBP->pcdev);
				break;
			}

			for (pcWhite = pBP->pcdev; '\000' != *pcWhite && !isspace(*pcWhite); ++pcWhite) {
				/* nada */
			}
			cKeep = *pcWhite;
			*pcWhite = '\000';
			(void)strcpy(pcOut, pBP->pcdev);
			*pcWhite = cKeep;
			break;
		case 'i':
			/* find newfs option, on primary ot alternate
			 * (they should be the same)
			 */
			pcTemp = NewfsOpts(pBP->pcrest);
			if ((char *)0 == pcTemp) {
				pcTemp = NewfsOpts(pBP->pBPswap->pcrest);
			}
			if ((char *)0 == pcTemp && fJudge) {
				if (MakeOpts(pBP->pBPswap->pcmount, pcOut)) {
					break;
				}
			}
			if ((char *)0 == pcTemp) {
				break;
			}
			(void)strcpy(pcOut, pcTemp);
			break;
		case 'm':
			(void)strcpy(pcOut, pBP->pcmount);
			break;
		case 'r':
			if (6 == iTableType) {
				if (0 == strncmp(pBP->pcdev, "/dev/", 5)) {
					sprintf(pcOut, "/dev/r%s", pBP->pcdev+5);
				} else {
					sprintf(pcOut, "%s", pBP->pcdev);
				}
				if (-1 == stat(pcOut, & stDev)) {
					sprintf(pcOut, "%s", pBP->pcdev);
				}
			} else {	/* type 7 */
				for (pcWhite = pBP->pcdev; '\000' != *pcWhite && !isspace(*pcWhite); ++pcWhite) {
					/* nada */
				}
				pcRaw = pcWhite;
				do {
					++pcRaw;
				} while ('\000' != *pcRaw && isspace(*pcRaw));
				cKeep = *pcWhite;
				*pcWhite = '\000';
				if (0 == strcmp("-", pcRaw)) {
					MapRaw(pcOut, pBP->pcdev);
				} else {
					sprintf(pcOut, "%s", pcRaw);
				}
				if (-1 == stat(pcOut, & stDev)) {
					sprintf(pcOut, "%s", pBP->pcdev);
				}
				*pcWhite = cKeep;
			}
			break;
		case 'K':
			if ((char *)0 == pcMethKeys) {
				break;
			}
			(void)strcpy(pcOut, pcMethKeys);
			break;
		case 'P':
			if ((char *)0 == pcMethParams) {
				break;
			}
			(void)strcpy(pcOut, pcMethParams);
			break;
		} break; }
		pcOut += strlen(pcOut);
	}
	*pcOut = '\000';
}

/* Run the given command						(ksb)
 * first exampand it to fill in the params, then print it to a shell
 */
static int
Run(pcFmt, pBP)
char *pcFmt;
SBP_ENTRY *pBP;
{
	auto char acCmd[MAXPATHLEN*4+10240];

	Expand(acCmd, pcFmt, pBP);
	if (fVerbose) {
		printf("%s\n", acCmd);
	}
	if (fExec) {
		return system(acCmd);
	}
	return 0;
}

/* make a mount-point, mode 555 root.0 or parent dir group		(ksb)
 */
static int
MkMtPoint(pBP)
SBP_ENTRY *pBP;
{
	if (fExec && IsDir(pBP->pcmount))
		return 0;
	return Run("[ -d \"%m\" ] || mkdir -p \"%m\"", pBP);
}


/* umount a partition and sync the filesystems				(ksb)
 */
static int
UmountPartition(pBP)
SBP_ENTRY *pBP;
{
	auto struct stat stTarget, stAbove;
	register char *pcSlash;
	register int i;

	if (!fExec || (char *)0 == (pcSlash = strrchr(pBP->pcmount, '/'))) {
		/* try it */
	} else {
		*pcSlash = '\000';
		i = stat(pBP->pcmount == pcSlash ? "/" : pBP->pcmount, & stAbove);
		*pcSlash = '/';
		if (-1 == i) {
			if (ENOENT == errno) {
				return /* not mounted - no parent */0;
			}
		} else if (-1 == stat(pBP->pcmount, & stTarget)) {
			if (ENOENT == errno) {
				return /* not mounted -- not exstant */0;
			}
		} else if (stTarget.st_dev == stAbove.st_dev) {
			return /* not mounted -- same device */0;
		}
	}
	return Run("umount %m", pBP);
}

/* run the locak mkfs interface						(ksb)
 */
static int
NewFs(pBP)
SBP_ENTRY *pBP;
{
#if defined(SUN5)
	return Run("echo \"yes\" |newfs %i %r", pBP);
#else
	return Run("newfs %i %r", pBP);
#endif
}

/* sync a partition with dump/restore
 * build newfs command -- we could guess at inode tune (?)
 * build mount command
 * build the dump | restore pipeline
 * remove the trash
 * unmount that Bad Boy.
 */
static int
CopyDumpRestore(pBP)
SBP_ENTRY *pBP;
{
	register int iRet;

	if (0 != (iRet = NewFs(pBP)))
		return iRet;
	if (0 != (iRet = MkMtPoint(pBP)))
		return iRet;
	if (0 != (iRet = Run("mount %a %d %m", pBP)))
		return iRet;

#if defined(FREEBSD)
	iRet = Run("%D 0af%K - %P %Xd | ( cd %m && %R -rf -)", pBP);
#else
	iRet = Run("%D 0f%K - %P %Xd | ( cd %m && %R -rf -)", pBP);
#endif
	Run("rm -f %m/restoresymtable", pBP);

	return iRet;
}

/* copy with cpio, the Old Way						(ksb)
 * newfs the partition
 * mount it
 * use cpio -p and find
 */
static int
CopyCpio(pBP)
SBP_ENTRY *pBP;
{
	register int iRet;

	if (0 != (iRet = NewFs(pBP)))
		return iRet;
	if (0 != (iRet = MkMtPoint(pBP)))
		return iRet;
	if (0 != (iRet = Run("mount %a %d %m", pBP)))
		return iRet;
	return Run("cd %Xm && find . -xdev -depth -print |cpio -pdm%K %P %m", pBP);
}

/* copy the raw partition with dd					(ksb)
 * this is _never_ recommended for filesystems
 */
static int
CopyDD(pBP)
SBP_ENTRY *pBP;
{
	register int iRet;

	if ((char *)0 == pcMethParams) {
		pcMethParams = "bs=126b";
	}
	if (0 != (iRet = Run("dd if=%Xr of=%d %P", pBP)))
		return iRet;
	if (0 != (iRet = Run("fsck -y %d", pBP)))
		return iRet;
	return Run("mount %d %m", pBP);
}

/* copy with the rsync program, only cool if the filesystem was		(ksb)
 * previously copied with something else to get it newfs'd
 */
static int
CopyRsync(pBP)
SBP_ENTRY *pBP;
{
	register int iRet;

	if (0 != (iRet = MkMtPoint(pBP)))
		return iRet;
	if (0 != (iRet = Run("mount %a %d %m", pBP)))
		return iRet;
	return Run("rsync -a -xHS --delete --numeric-ids %P %Xm/ %m", pBP);
}

/* copy with GNU tar, old style tar won't honor device boundries	(ksb)
 * or copy device nodes.
 */
static int
CopyTar(pBP)
SBP_ENTRY *pBP;
{
	register int iRet;

	if (0 != (iRet = NewFs(pBP)))
		return iRet;
	if (0 != (iRet = MkMtPoint(pBP)))
		return iRet;
	if (0 != (iRet = Run("mount %a %d %m", pBP)))
		return iRet;
	return Run("tar --create --file - --one-file-system -C %Xm . | tar xpf - --sparse %P -C %m", pBP);
}

/* check the configuration for simple (and other) problems		(ksb)
 * we check:
 *	- radically different size partitions (primary and backup)
 *	- mulitple use of a backup partition (as backup or primary)
 *	- missing raw names for primary partitions on a Sun (not fsck'd)
 *	- no superblock on the backup device !? XXX (see softupdates above)
 *	- ??? LLL
 */
static int
MetaCheck(pBP)
SBP_ENTRY *pBP;
{
	printf("%s: sane is not implemented (yet).\n", progname);
	exit(2);
}

/* what we know how to do
 */
static SBP_METHOD aSMList[10] = {
	{ "dump", "use filesystem dump and restore to copy partitions",
		CopyDumpRestore, UmountPartition, ParamDump},
	{ "cpio", "use cpio to copy partitions",
		CopyCpio,	 UmountPartition, ParamDump},
	{   "dd", "try to copy the block device with dd",
		CopyDD,		 UmountPartition, ParamDash},
	{  "tar", "copy the partition data with GNU tar",
		CopyTar,	 UmountPartition, ParamDash},
	{"rsync", "use the rsync tool to sync the partitions",
		CopyRsync,	 UmountPartition, ParamDash},
	{"sane", "just check the configuration for sanity",
		MetaCheck,	 UmountPartition,  ParamDash},
	{(char *)0, "ksb's credit line. Copyright K.S.Brasundorf 1999 - OpenSource",
		(int (*)())0, (int (*)())0, (void (*)())0}
		
};

/* explain things to the SA trying to use us				(ksb)
 */
void
MethHelp(fp)
FILE *fp;
{
	register int i, iLen, iTemp;
	register char *pcMeth;

	iLen = 0;
	for (i = 0; (char *)0 != (pcMeth = aSMList[i].pcmeth); ++i) {
		iTemp = strlen(pcMeth);
		if (iTemp > iLen)
			iLen = iTemp;
	}
	iLen += 1;
	for (i = 0; (char *)0 != (pcMeth = aSMList[i].pcmeth); ++i) {
		fprintf(fp, "%-*s %s\n", iLen, pcMeth, aSMList[i].pchelp);
	}
	fflush(fp);
}

static SBP_METHOD *pSMUse = aSMList;

void
SetHow(pcHow)
char *pcHow;
{
	register char *pcMeth, *pcColon;
	register int i, iLen;

	if ('?' == pcHow[0]) {
		MethHelp(stdout);
		exit(0);
	}
	if ((char *)0 != (pcColon = strchr(pcHow, ':'))) {
		iLen = pcColon - pcHow;
		++pcColon;
	} else {
		iLen = strlen(pcHow);
	}
	for (i = 0; (char *)0 != (pcMeth = aSMList[i].pcmeth); ++i) {
		if (0 == strncmp(pcMeth, pcHow, iLen)) {
			pSMUse = & aSMList[i];
			if ((char *)0 != pcColon) {
				(pSMUse->pfiparams)(pcColon);
			}
			return;
		}
	}
	fprintf(stderr, "%s: %s: unknown sync method, try `?' for a list\n", progname, pcHow);
	exit(1);
}

/* build the exchanged vfstab and install it				(ksb)
 */
static void
BackupFstab()
{
	auto char acInstCmd[MAXPATHLEN+4];
	register FILE *fpInstall;

	sprintf(acInstCmd, "%s -q - %s%s", acInstallPath, pcMtPoint, acFstabPath);
	if (fVerbose) {
		printf("%s <<\\!\n", acInstCmd);
		DumpFs(stdout);
		printf("!\n");
	}
	if (fExec) {
		fpInstall = popen(acInstCmd, "w");
		DumpFs(fpInstall);
		pclose(fpInstall);
	}
}

/* count the number of this character in the string			(ksb)
 */
static int
CountEm(pcStr, cThing)
char *pcStr;
int cThing;
{
	register int iRet;

	for (iRet = 0; '\000' != *pcStr; ++pcStr) {
		if (cThing == *pcStr)
			++iRet;
	}
	return iRet;
}

/* the mount points with fewer slashes and shorter names go first	(ksb)
 */
static int
PartByLen(ppBPLeft, ppBPRight)
SBP_ENTRY **ppBPLeft, **ppBPRight;
{
	register char *pcLeft, *pcRight;
	register int iCmp;

	pcLeft = ppBPLeft[0]->pcmount;
	pcRight = ppBPRight[0]->pcmount;
	iCmp = CountEm(pcLeft, '/') - CountEm(pcRight, '/');
	if (0 != iCmp)
		return iCmp;
	iCmp = strlen(pcLeft) - strlen(pcRight);
	if (0 != iCmp)
		return iCmp;
	return strcmp(pcLeft, pcRight); 
}

/* remove the trailing slash that zsh/bash/ksh file completion		(ksb)
 * puts on a parameter
 */
static void
TrimTrailing(pcPath)
char *pcPath;
{
	register int i;

	for (i = strlen(pcPath); i-- > 0; pcPath[i] = '\000') {
		if ('/' != pcPath[i])
			return;
	}
}

/* sync the partitions in the list, either primary or backup		(ksb)
 * by default (zero listed) sync all
 */
void
DoSync(argc, argv, pBPList)
int argc;
char **argv;
SBP_ENTRY *pBPList;
{
	register int i, iMtLen, iErr, iTrip;
	register SBP_ENTRY *pBPScan, **ppBPOrder;
	register char *pcMatch;

	iErr = 0;
	iMtLen = strlen(pcMtPoint);
	/* we should remove trailing slash's on pcMatch (or ignore them)
	 */
	for (i = 0; i < argc; ++i) {
		register char *pcCand;
		if (0 == strncmp(argv[i], pcMtPoint, iMtLen)) {
			pcMatch = argv[i] + iMtLen;
			if ('/' == *pcMatch) {
				++pcMatch;
			}
		} else {
			pcMatch = argv[i] + 1;
		}
		TrimTrailing(pcMatch);
		for (pBPScan = pBPList; (SBP_ENTRY *)0 != pBPScan; pBPScan = pBPScan->pBPnext) {
			pcCand = pBPScan->pcmount+iMtLen;
			if ('/' == *pcCand) {
				++pcCand;
			}
			if (0 == strcmp(pcCand, pcMatch))
				break;
		}
		if ((SBP_ENTRY *)0 != pBPScan) {
			pBPScan->wflags |= 1;
			continue;
		}
		fprintf(stderr, "%s: %s </%s>: can't find backup partition\n", progname, argv[i], pcMatch);
		iErr = 1;
	}
	if (0 != iErr) {
		exit(iErr);
	}
	iTrip = 0;
	for (pBPScan = pBPList; (SBP_ENTRY *)0 != pBPScan; pBPScan = pBPScan->pBPnext) {
		if (0 != argc && 0 == pBPScan->wflags) {
			continue;
		}
		++iTrip;
	}
	/* nothing to sync in fstable, I guess
	 */
	if (0 == iTrip) {
		if (fVerbose) {
			printf("%s: %s: nothing to sync\n", progname, pcFstab);
		}
		return;
	}

	/* find the order to do them in, sigh
	 */
	ppBPOrder = (SBP_ENTRY **)calloc((iTrip|7)+1, sizeof(SBP_ENTRY *));
	if ((SBP_ENTRY **)0 == ppBPOrder) {
		fprintf(stderr, "%s: calloc: %d,%d: %s\n", progname, (iTrip|7)+1, sizeof(SBP_ENTRY *), strerror(errno));
		exit(3);
	}
	iTrip = 0;
	for (pBPScan = pBPList; (SBP_ENTRY *)0 != pBPScan; pBPScan = pBPScan->pBPnext) {
		if (0 != argc && 0 == pBPScan->wflags) {
			continue;
		}
		ppBPOrder[iTrip++] = pBPScan;
	}
	ppBPOrder[iTrip] = (SBP_ENTRY *)0;
	qsort((void *)ppBPOrder, iTrip, sizeof(SBP_ENTRY *), PartByLen);

	/* unmount backups, we may have been interrupted
	 */
	for (i = 0; i < iTrip; ++i) {
		(pSMUse->pfiumount)(ppBPOrder[iTrip - 1 - i]);
	}

	/* make sure the top level mount-point exists
	 * if one just asks for /usr/local to be sync'd we'll
	 * make a /backup/usr/local mount point to sync it on,
	 * but the extra dirs don't hurt anyone.
	 * dd -> "dd if=%Xd of=%d" (in another routine, this is mounted)
	 */
	for (i = 0; i < iTrip; ++i) {
		if ((char *)0 != pcBefore && 0 != Run(pcBefore, ppBPOrder[i])) {
			break;
		}
		if (0 != (pSMUse->pficopy)(ppBPOrder[i])) {
			break;
		}
	}
	/* if we did "/", (the first on in the list) then we need to
	 * install a new /backup/etc/fstab with the partitions
	 * swapped around -- ksb
	 */
	if (i > 0 && 0 == strcmp(pcMtPoint, ppBPOrder[0]->pcmount)) {
		BackupFstab();
	}
	/* trap a command to fix stuff before we go
	 */
	if (i > 0 && (char *)0 != pcCleanup) {
		(void)Run(pcCleanup, ppBPOrder[0]);
	}
	while (i-- > 0) {
		if ((char *)0 != pcAfter && 0 != Run(pcAfter, ppBPOrder[i])) {
			break;
		}
		(pSMUse->pfiumount)(ppBPOrder[i]);
	}
	sync();
}
