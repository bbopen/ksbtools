/* $Id: fslist.c,v 3.6 1999/05/23 18:44:36 ksb Exp $
 *
 * Stolen from the entomb/entomb/fslist.c code whole sale,
 * of course I wrote that too -- ksb
 *
 * $Compile: ${cc-cc} -DTEST -I../libtomb -o %F %s %f
 * $Compile: ${cc-cc} -DTEST -I../libtomb -o %F -DHAS_GETMNT %f
 * $Compile: ${cc-cc} -DTEST -I../libtomb -o %F -DHAS_GETMNTENT %f
 * $Compile: ${cc-cc} -DTEST -I../libtomb -o %F -DHAS_MTAB %f
 * $Compile: ${cc-cc} -DTEST -I../libtomb -o %F -DHAS_VMNT %f
 * $Compile: ${cc-cc} -DTEST -I../libtomb -o %F -DHAS_GETMNTINFO %f
 */
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>

#include "main.h"
#include "machine.h"
#include "fslist.h"

void AppendMntFs(char *, char *);

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
#include <stdlib.h>
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

/* no_way_to_get_mounted_file_systems no_way_to_get_mounted_file_systems;; */
#endif /* free bsd (386BSD) getmntinfo */
#endif /* IBM vmounts */
#endif /* getmntent */
#endif /* /etc/mtab */
#endif /* getmnt() */

extern char *progname;

/* append information about mounted filesystems to pcBuf
 */
void GetMntFs(char *pcBuf)
{
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

	strcat(pcBuf, "f:");
	pcBuf += 2;

#if HAS_GETMNT
	iStart = 0;
	while (0 < (n = getmnt(&iStart, & afsBuf[0], sizeof(afsBuf), NOSTAT_MANY, (char *)0))) {
		while (n-- > 0) {
			AppendMntFs(pcBuf, afsBuf[n].fd_req.path);
		}
	}
#endif
#if HAS_GETMNTENT
#if USE_SYS_MNTTAB_H
	if ((FILE *)0 == (pfME = fopen("/etc/mnttab", "r"))) {
		return;
	}
	while (0 == getmntent(pfME, & ME)) {
		if ('.' == ME.mnt_mountp[0] && '\000' == ME.mnt_mountp[1]) {
			continue;	/* ignore me */
		}
		if (0 == strcmp(ME.mnt_fstype, "ignore")) {
			continue;	/* ignore me */
		}
		AppendMntFs(pcBuf, ME.mnt_mountp);
	}
	(void)fclose(pfME);
#else
	if ((FILE *)0 == (pfME = setmntent(MTAB_PATH, "r"))) {
		return;
	}
	while ((struct mntent *)0 != (pME = getmntent(pfME))) {
		if ('.' == pME->mnt_dir[0] && '\000' == pME->mnt_dir[1]) {
			continue;	/* ignore me */
		}
		if (0 == strcmp(pME->mnt_type, "ignore")) {
			continue;	/* ignore me */
		}
		AppendMntFs(pcBuf, pME->mnt_dir);
	}
	(void)endmntent(pfME);
#endif
#endif
#if HAS_MTAB
	if (-1 == (fdMtab = open("/etc/mtab", 0, 0))) {
		return;
	}
	while (sizeof(struct mtab) == read(fdMtab, &mtBuf, sizeof(mtBuf))) {
		if ('\000' == mtBuf.m_path[0])
			continue;
		AppendMntFs(pcBuf, mtBuf.m_path);
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
			fprintf(stderr, "%s: mntctl error: %s\n", progname, strerror(errno));
			return;
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
		/* "%s:%s", (char *)vmt2dataptr(v, VMT_HOST),
		 * (char *)vmt2dataptr(v, VMT_OBJECT)
		 */
		pcDir = (char *)vmt2dataptr(v, VMT_STUB);
		if (0 == (v->vmt_flags & MNT_REMOTE))
			continue;
		AppendMntFs(pcBuf, pcDir);
        }
	(void) free((char *)vt);
#endif
#if HAS_GETMNTINFO
	if (-1 ==  (iTable = getmntinfo(& pSF, MOUNT_NONE))) {
                return;
        }
	for (n = 0; n < iTable; ++n) {
		if ('\000' == pSF[n].f_mntonname[0])
			continue;
		AppendMntFs(pcBuf, pSF[n].f_mntonname);
	}
#endif
}

#if defined(USERFS_PREFIX)
/* emulate the original behavior of GetMntFs:  iff the path begins	(bj)
 * with USERFS_PREFIX, append the path (without the prefix) to the
 * buffer
 */
void
AppendMntFs(pcBuf, pcPath)
char *pcBuf;
char *pcPath;
{
	register int iLen;
	static int iPrefixLen;
	static char *pcPrefix;

	if (0 == iPrefixLen) {
		pcPrefix = USERFS_PREFIX;
		iPrefixLen = strlen(pcPrefix);
	}

	if (0 != strncmp(pcPath, pcPrefix, iPrefixLen)) {
		return;
	}
	iLen = strlen(pcBuf);
	pcBuf += iLen;
	if (0 != iLen) {
		*pcBuf++ = ' ';
	}
	strcpy(pcBuf, pcPath + iPrefixLen);
}
#endif /* USERFS_PREFIX */
