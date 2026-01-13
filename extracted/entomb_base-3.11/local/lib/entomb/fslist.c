/* $Id: fslist.c,v 3.11 2003/04/20 20:24:40 ksb Exp $
 *
 * $Compile: ${cc-cc} -DTEST -I../libtomb -o %F %s %f
 * $Compile: ${cc-cc} -DTEST -I../libtomb -o %F -DHAS_GETMNT %f
 * $Compile: ${cc-cc} -DTEST -I../libtomb -o %F -DHAS_GETMNTENT %f
 * $Compile: ${cc-cc} -DTEST -I../libtomb -o %F -DHAS_MTAB %f
 * $Compile: ${cc-cc} -DTEST -I../libtomb -o %F -DHAS_VMNT %f
 * $Compile: ${cc-cc} -DTEST -I../libtomb -o %F -DHAS_GETMNTINFO %f
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "machine.h"
#include "libtomb.h"
#include "fslist.h"

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if HAS_GETMNT
/* Ultrix, for example */
#include <sys/mount.h>
#else

#if HAS_MTAB
/* 4.3bsd */
#include <mtab.h>
#else

#if HAS_GETMNTENT
/* Dynix 4.3 (PUCC) and the like */
#if USE_SYS_MNTTAB_H
#include <sys/mnttab.h>
#else
#include <mntent.h>
#endif
#else

#if HAS_VMNT
/* RS6000 systems has vmount */
#include <dirent.h>
#if	!defined(_KERNEL)
#define _KERNEL	1
#endif
#include <sys/file.h>
#undef	_KERNEL
#include <sys/mntctl.h>
#include <sys/sysmacros.h>
#include <sys/vattr.h>
#include <sys/vmount.h>
#include <procinfo.h>
#include <nfs/rnode.h>

/*
 * AIX 3.1.[357] doesn't supply the necessary cdrnode.h.
 */

struct cdrnode {			/* ZZZ see if used */
	caddr_t		f1[4];
	struct gnode	f2;
	dev_t		f3;
	ino_t		cn_inumber;	/* inode number */
	caddr_t		f4;
	cnt_t		f5[2];
	u_short		f6;
	uint		f7[2];
	uchar		f8[3];
	off_t		cn_size;	/* size of file in bytes */
};
#else

#if HAS_GETMNTINFO
/* 368BSD */
#include <sys/mount.h>
#else

no_way_to_get_mounted_file_systems no_way_to_get_mounted_file_systems;;
#endif /* free bsd (386BSD) getmntinfo */
#endif /* IBM vmounts */
#endif /* getmntent */
#endif /* /etc/mtab */
#endif /* getmnt() */

#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif
extern char *progname;

static char acDefTomb[] =	/* default tomb name ("tomb" is cool)	*/
	TOMB_NAME;

/* make a linked list of filesystem information				(ksb)
 */
TombInfo *
FsList(iGid, theDev)
int iGid;
dev_t theDev;		/* when searching for a particular device */
{
	static TombInfo *pTIHead = (TombInfo *)0;
	static char acTomb[MAXPATHLEN+1];
	struct stat statBuf;
#if HAS_GETMNT
	auto int iStart, n;
	auto struct fs_data afsBuf[16];
#endif
#if HAS_GETMNTENT
#if USE_SYS_MNTTAB_H
	struct mnttab ME;
#else
	struct mntent *pME;
#endif
	FILE *pfME;
#endif
#if HAS_MTAB
	int fdMtab;
	auto struct mtab mtBuf;
#endif
#if HAS_VMNT
	auto char acFsName[1024+1+256+1];
	register int nm;
	register char *pcDir;
	register struct vmount *v, *vt;
	register unsigned sz;
#endif
#if HAS_GETMNTINFO
	auto struct statfs *pSF;
	register int iTable, n;
#endif

	/*
	** If we're being called a second time, free the stuff we allocated
	** the first time.
	*/
	while ((TombInfo *)0 != pTIHead) {
		pTIHead = TIDelete(pTIHead);
	}

#if HAS_GETMNT
	iStart = 0;
	while (0 < (n = getmnt(&iStart, & afsBuf[0], sizeof(afsBuf), NOSTAT_MANY, (char *)0))) {
		while (n-- > 0) {
			(void)strcpy(acTomb, afsBuf[n].fd_req.path);
			if ('/' == acTomb[0] && '\000' == acTomb[1]) {
				(void)strcat(acTomb, acDefTomb);
			} else {
				(void)strcat(acTomb, "/");
				(void)strcat(acTomb, acDefTomb);
			}

			if (-1 == stat(acTomb, &statBuf)) {
				if (NOT_A_DEV != theDev && -1 != stat(afsBuf[n].fd_req.path, &statBuf) && theDev == statBuf.st_dev) {
					break;
				}
				continue;
			}

			if (iGid != statBuf.st_gid) {
				if (NOT_A_DEV != theDev && theDev == statBuf.st_dev) {
					break;
				}
				continue;
			}

			/*
			** If we're just looking for a specific device,
			** we'll break out of this loop when we find it.
			*/
			if (NOT_A_DEV != theDev) {
				if (theDev != statBuf.st_dev) {
					continue;
				}
				pTIHead = TINew(pTIHead, acTomb, afsBuf[n].fd_req.devname, &statBuf);
				break;
			}

			pTIHead = TINew(pTIHead, acTomb, afsBuf[n].fd_req.devname, &statBuf);
		}
	}
#endif
#if HAS_GETMNTENT
#if USE_SYS_MNTTAB_H

#if !defined(PATH_MNTTAB)
#define	PATH_MNTTAB	"/etc/mnttab"
#endif
	if ((FILE *)0 == (pfME = fopen(PATH_MNTTAB, "r"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, PATH_MNTTAB, strerror(errno));
		exit(NOT_ENTOMBED);
	}
	while (0 == getmntent(pfME, & ME)) {
		if ('.' == ME.mnt_mountp[0] && '\000' == ME.mnt_mountp[1]) {
			continue;	/* ignore me */
		}
		/* a better hack than the old one to ignore
		 * automount shadows -- ksb
		 */
		if (0 == strcmp(ME.mnt_fstype, "autofs")) {
			/* not sure about this one XXX ZZZ */
			continue;
		}
		if ((char *)0 != hasmntopt(&ME, "ignore")) {
			continue;
		}

		(void)strcpy(acTomb, ME.mnt_mountp);
		if ('/' == acTomb[0] && '\000' == acTomb[1]) {
			(void)strcat(acTomb, acDefTomb);
		} else {
			(void)strcat(acTomb, "/");
			(void)strcat(acTomb, acDefTomb);
		}

		if (-1 == stat(acTomb, &statBuf)) {
			if (NOT_A_DEV != theDev && -1 != stat(ME.mnt_mountp, &statBuf) && theDev == statBuf.st_dev) {
				break;
			}
			continue;
		}

		if (iGid != statBuf.st_gid) {
			if (NOT_A_DEV != theDev && theDev == statBuf.st_dev) {
				break;
			}
			continue;
		}


		/*
		** If we're just looking for a specific device,
		** we'll break out of this loop when we find it.
		*/
		if (NOT_A_DEV != theDev) {
			if (theDev != statBuf.st_dev) {
				continue;
			}
			pTIHead = TINew(pTIHead, acTomb, ME.mnt_special, &statBuf);
			break;
		}

		pTIHead = TINew(pTIHead, acTomb, ME.mnt_special, &statBuf);
	}
	(void)fclose(pfME);
#else
#if !defined(PATH_MNTTAB)
#if HPUX
#define PATH_MNTTAB	"/etc/mnttab"
#else
#define PATH_MNTTAB	"/etc/mtab"
#endif
#endif
	if ((FILE *)0 == (pfME = setmntent(PATH_MNTTAB, "r"))) {
		fprintf(stderr, "%s: setmntent: %s: %s\n", progname, PATH_MNTTAB, strerror(errno));
		exit(NOT_ENTOMBED);
	}
	while ((struct mntent *)0 != (pME = getmntent(pfME))) {
		if ('.' == pME->mnt_dir[0] && '\000' == pME->mnt_dir[1]) {
			continue;	/* ignore me */
		}
		if (0 == strcmp(pME->mnt_type, "ignore")) {
			continue;	/* ignore me */
		}

		(void)strcpy(acTomb, pME->mnt_dir);
		if ('/' == acTomb[0] && '\000' == acTomb[1]) {
			(void)strcat(acTomb, acDefTomb);
		} else {
			(void)strcat(acTomb, "/");
			(void)strcat(acTomb, acDefTomb);
		}

		if (-1 == stat(acTomb, &statBuf)) {
			if (NOT_A_DEV != theDev && -1 != stat(pME->mnt_dir, &statBuf) && theDev == statBuf.st_dev) {
				break;
			}
			continue;
		}

		if (iGid != statBuf.st_gid) {
			if (NOT_A_DEV != theDev && theDev == statBuf.st_dev) {
				break;
			}
			continue;
		}


		/*
		** If we're just looking for a specific device,
		** we'll break out of this loop when we find it.
		*/
		if (NOT_A_DEV != theDev) {
			if (theDev != statBuf.st_dev) {
				continue;
			}
			pTIHead = TINew(pTIHead, acTomb, pME->mnt_fsname, &statBuf);
			break;
		}

		pTIHead = TINew(pTIHead, acTomb, pME->mnt_fsname, &statBuf);
	}
	(void)endmntent(pfME);
#endif
#endif
#if HAS_MTAB
	if (-1 == (fdMtab = open("/etc/mtab", 0, 0))) {
		fprintf(stderr, "%s: /etc/mtab: %s\n", progname, strerror(errno));
		exit(1);
	}
	while (sizeof(struct mtab) == read(fdMtab, &mtBuf, sizeof(mtBuf))) {
		if ('\000' == mtBuf.m_path[0])
			continue;
		(void)strcpy(acTomb, mtBuf.m_path);
		if ('/' == acTomb[0] && '\000' == acTomb[1]) {
			(void)strcat(acTomb, acDefTomb);
		} else {
			(void)strcat(acTomb, "/");
			(void)strcat(acTomb, acDefTomb);
		}

		if (-1 == stat(acTomb, &statBuf)) {
			continue;
		}

		if (iGid != statBuf.st_gid) {
			if (NOT_A_DEV != theDev && theDev == statBuf.st_dev) {
				break;
			}
			continue;
		}

		/*
		** If we're just looking for a specific device,
		** we'll break out of this loop when we find it.
		*/
		if (NOT_A_DEV != theDev) {
			if (theDev != statBuf.st_dev) {
				continue;
			}
			pTIHead = TINew(pTIHead, acTomb, mtBuf.m_dname, &statBuf);
			break;
		}

		pTIHead = TINew(pTIHead, acTomb, mtBuf.m_dname, &statBuf);
	}
	(void)close(fdMtab);
#endif
#if HAS_VMNT
	/* read in the current vmount table
	 */
	for (sz = sizeof(struct vmount);;) {
		if ((vt = (struct vmount *)malloc(sz)) == NULL) {
			fprintf(stderr, "%s: no space for vmount table\n", progname);
			break;
		}
		nm = mntctl(MCTL_QUERY, sz, vt);
		if (nm < 0) {
			fprintf(stderr, "%s: mntctl error: %s\n", progname, sys_errlist[errno]);
			return 0;
		}
		if (nm > 0) {
			if (vt->vmt_revision != VMT_REVISION)
				fprintf(stderr, "%s: stale file system, rev %d != %d\n", progname, vt->vmt_revision, VMT_REVISION);
			break;
		}
		sz = (unsigned)vt->vmt_revision;
	}

	/* Scan the vmount structures and build Mtab.
	 */
	for (v = vt; nm--; v = (struct vmount *)((char *)v + v->vmt_length)) {
		pcDir = (char *)vmt2dataptr(v, VMT_STUB);
		if (v->vmt_flags & MNT_REMOTE) {
			(void) sprintf(acFsName, "%s:%s", (char *)vmt2dataptr(v, VMT_HOST), (char *)vmt2dataptr(v, VMT_OBJECT));
		} else {
			(void) strcpy(acFsName, (char *)vmt2dataptr(v, VMT_OBJECT));
		}

		(void)strcpy(acTomb, pcDir);
		if ('/' == acTomb[0] && '\000' == acTomb[1]) {
			(void)strcat(acTomb, acDefTomb);
		} else {
			(void)strcat(acTomb, "/");
			(void)strcat(acTomb, acDefTomb);
		}

		if (-1 == stat(acTomb, &statBuf)) {
			if (NOT_A_DEV != theDev && -1 != stat(pcDir, &statBuf) && theDev == statBuf.st_dev) {
				break;
			}
			continue;
		}

		if (iGid != statBuf.st_gid) {
			if (NOT_A_DEV != theDev && theDev == statBuf.st_dev) {
				break;
			}
			continue;
		}

		/*
		** If we're just looking for a specific device,
		** we'll break out of this loop when we find it.
		*/
		if (NOT_A_DEV != theDev) {
			if (theDev != statBuf.st_dev) {
				continue;
			}
			pTIHead = TINew(pTIHead, acTomb, acFsName, &statBuf);
			break;
		}

		pTIHead = TINew(pTIHead, acTomb, acFsName, &statBuf);
        }
	(void) free((char *)vt);
#endif
#if HAS_GETMNTINFO
#if !defined(ENTOMB_MOUNT_OPTS)
#if defined(MNT_WAIT)
#define ENTOMB_MOUNT_OPTS	MNT_WAIT
#elif defined(MOUNT_NONE)
#define ENTOMB_MOUNT_OPTS	MOUNT_NONE
#else
#define ENTOMB_MOUNT_OPTS	0
#endif
#endif
	if (-1 ==  (iTable = getmntinfo(& pSF, ENTOMB_MOUNT_OPTS))) {
                fprintf(stderr, "%s: getmntinfo: %s\n", progname, strerror(errno));
                exit(1);
        }
	for (n = 0; n < iTable; ++n) {
		if ('\000' == pSF[n].f_mntonname[0])
			continue;
		(void)strcpy(acTomb, pSF[n].f_mntonname);
		if ('/' == acTomb[0] && '\000' == acTomb[1]) {
			(void)strcat(acTomb, acDefTomb);
		} else {
			(void)strcat(acTomb, "/");
			(void)strcat(acTomb, acDefTomb);
		}

		if (-1 == stat(acTomb, &statBuf)) {
			continue;
		}

		if (iGid != statBuf.st_gid) {
			if (NOT_A_DEV != theDev && theDev == statBuf.st_dev) {
				break;
			}
			continue;
		}

		/*
		** If we're just looking for a specific device,
		** we'll break out of this loop when we find it.
		*/
		if (NOT_A_DEV != theDev) {
			if (theDev != statBuf.st_dev) {
				continue;
			}
			pTIHead = TINew(pTIHead, acTomb, pSF[n].f_mntfromname, &statBuf);
			break;
		}

		pTIHead = TINew(pTIHead, acTomb, pSF[n].f_mntfromname, &statBuf);
	}
#endif

	return pTIHead;
}


/* Create a new TI node, prepend it to the given list,			(ksb)
 * and return a pointer to the new list.
 */
TombInfo *
TINew(pTI, pcDir, pcDev, pstat)
register TombInfo *pTI;
register char *pcDir;
register char *pcDev;
register struct stat *pstat;
{
	register TombInfo *pTINew = (TombInfo *)malloc((unsigned int)sizeof(TombInfo));

	if ((TombInfo *)0 == pTINew) {
		(void)fprintf(stderr, "%s: malloc: out of memory\n", "TINew");
		exit(NOT_ENTOMBED);
	}

	pTINew->pcDir = (char *)malloc((unsigned int)(strlen(pcDir)+1));
	(void)strcpy(pTINew->pcDir, pcDir);

	if ('/' != *pcDev && (char *)0 == strchr(pcDev, ':')) {
		pTINew->pcDev = (char *)malloc((unsigned int)(strlen(pcDev)+1+sizeof("/dev/")));
		strcpy(pTINew->pcDev, "/dev/");
		strcat(pTINew->pcDev, pcDev);
	} else {
		pTINew->pcDev = (char *)malloc((unsigned int)(strlen(pcDev)+1));
		(void)strcpy(pTINew->pcDev, pcDev);
	}

	pTINew->tdev = pstat->st_dev;
	pTINew->tino = pstat->st_ino;
	/* the Dir changes when we cd into the tomb,
	 * this stays the same so we can mention it on error messages
	 * and free it
	 */
	pTINew->pcFull = pTINew->pcDir;
	pTINew->pTINext = pTI;

	return pTINew;
}


/* Delete the given node and return a pointer to the next node.		(ksb)
 */
TombInfo *
TIDelete(pTI)
register TombInfo *pTI;
{
	TombInfo *pTINext = (TombInfo *)0;

	if ((TombInfo *)0 != pTI) {
		pTINext = pTI->pTINext;

		if ((char *)0 != pTI->pcDir && pTI->pcDir == pTI->pcFull) {
			(void)free(pTI->pcDir);
		}
		if ((char *)0 != pTI->pcDir) {
			(void)free(pTI->pcDir);
		}
		(void)free((char *)pTI);
	}

	return pTINext;
}


#ifdef TEST
char *progname = "test-fslist";

int
main(argc, argv)
int argc;
char **argv;
{
	register TombInfo *pTI;

	if (argc != 3) {
		fprintf(stderr, "%s: major minor\n", progname);
		exit(1);
	}
	for (pTI = FsList(atoi(argv[1]), (dev_t)atoi(argv[2])); (TombInfo *)0 != pTI; pTI = pTI->pTINext) {
		fprintf(stderr, "%s: dir=%s fsname=%s\n", progname, pTI->pcDir, pTI->pcDev);
	}
	exit(0);
}
#endif
