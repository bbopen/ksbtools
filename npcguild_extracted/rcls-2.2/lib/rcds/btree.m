#!mkcmd
# $Id: btree.m,v 2.6 2000/06/29 20:22:37 ksb Exp $
# A BTREE unit driver for our customer data services			(ksb)
# Uses the Berkeley db (3.0.55) to keep a nifty BTree of key/value pairs
# for the Customer.  We open the tree with a subtree
#
from '<sys/types.h>'
from '<sys/param.h>'
from '<sys/time.h>'
from '<sys/stat.h>'
from '<strings.h>'
from '<fcntl.h>'
from '<stdio.h>'
from '<stdlib.h>'
from '<unistd.h>'
from '<stdlib.h>'
from '<limits.h>'
from '<db.h>'

from '"rcds_callback.h"'

require "std_help.m" "std_version.m"
require  "util_errno.m" "util_ppm.m"
require "btree-post.m"
key "version_args" 2 {
	2 "btree_rcsid"
}
basename "btree" ""
routine "setup"
cleanup "unit_register(& UCBtree);"


integer 'p' {
	named "iPageSize"
	init  "1024"
	help "data base page size metric (1k def, 512 to 16k range)"
}
boolean 'r' {
	named "fRecovery"
	track "fGaveRecovery"
	update "%n = fGaveRecovery ? DB_RECOVER_FATAL : DB_RECOVER;"
	help "run recovery, or given more than once run recovery_fatal"
}

char* 'R' {
	named "pcRoot"
	init '"."'
	help "btree DB root (default to .)"
}

%i
static char btree_rcsid[] =
	"$Id: btree.m,v 2.6 2000/06/29 20:22:37 ksb Exp $";

#if !defined(MAX_B_CACHE)
#define MAX_B_CACHE	512	/* must be a Power of 2			*/
#endif

/* We can have up to MAX_B_CACHE trees open.
 */
typedef struct BHnode {		/* See after_func for init code		*/
	unsigned int uihash;	/* hash value				*/
	char acpath[MAXPATHLEN+4];/* name for this tree			*/
	unsigned int uilru;	/* least recently used counter		*/
	DB *pDBopen;
} BTREE_HASH;
static BTREE_HASH BHIndex[MAX_B_CACHE];
static unsigned int uiLRU;	/* global least recently used		*/
static DB_ENV *pDBEBtree;	/* upgrade to Berk DB 3.0.55		*/
static DBT DBTKey, DBTSData;
%%

%c
/* Index routines (to find a Customer's data records)			(ksb)
 */

/* get out now
 */
static void
BtreePanic(DB_ENV *pDBEWhich, int iErr)
{
	fprintf(stderr, "%s: %s: database recovery needed %d\n", progname, btree_progname, iErr);
	/* XXX should syslog or labc here -- ksb */
	exit(125);
}

/* make a hash to try to quickly find an already open DB		(ksb)
 */
static int
PathHash(char *pcPath)
{
	register int iRet, iSum;

	for (iRet = iSum = 0; *pcPath; ++pcPath) {
		if ('/' == *pcPath)
			continue;
		iSum += *pcPath;
		iRet <<= 2;
		iRet ^= *pcPath;
	}
	return (iRet ^ iSum);
}

/* Find the needed btree for the given uuid name, open it and		(ksb)
 * return the open (DB *).
 */
static DB *
FindTree(char *pcPath, char *pcUnit)
{
	register unsigned int uiHash;
	register int i, iCur, iLooser, iRet;
	register BTREE_HASH *pBH;
	register unsigned int uiAge;
	register char *pcTail, *pcSubUnit;
	register int iFlags;

	iFlags = DB_CREATE;
	if ((char *)0 != (pcSubUnit = unit_spec(pcUnit))) {
		*pcSubUnit++ = '\000';
		if ('\000' == *pcSubUnit) {
			pcSubUnit = (char *)0;
			iFlags = DB_RDONLY;
		}
	}
	pcTail = strchr(pcPath, '\000');
	*pcTail++ = '/';
	(void)strcpy(pcTail, pcUnit);
	if ((char *)0 != pcSubUnit) {
		pcTail = strchr(pcTail, '\000');
		pcSubUnit[-1] = *pcTail = ':';
		(void)strcpy(pcTail+1, pcSubUnit);
	}

	/* Find someone with our same hash value and path, or look though the
	 * whole cache from the place we hashed to until we find either an
	 * empty slot, or loop back around to the beginning.  Find the oldest
	 * entry (iLooser) or an empty entry.
	 */
	uiHash = PathHash(pcPath);
	iLooser = -1;
	iCur = uiHash % MAX_B_CACHE;
	pBH = & BHIndex[iCur];
	for (i = 0; i < MAX_B_CACHE; ++i) {
		if ('\000' == pBH->acpath[0]) {
			iLooser = iCur;
			break;
		}
		if (pBH->uihash == uiHash && 0 == strcmp(pcPath, pBH->acpath)) {
			pBH->uilru = uiLRU;
			return pBH->pDBopen;
		}
		if (-1 == iLooser) {
			iLooser = i;
			uiAge = pBH->uilru;
		} else if (pBH->uilru < uiAge) {
			iLooser = i;
			uiAge = pBH->uilru;
		}

		/* advance to next possible location
		 */
		++pBH;
		if (++iCur == MAX_B_CACHE) {
			iCur = 0;
			pBH = & BHIndex[0];
		}
	}

	/* Update least recently used counter, when we would wrap back to
	 * 0 we wrap to MAX_B_CACHE and clear the high bits on the existing
	 * LRU table.
	 */
	if (0U == ++uiLRU) {
		for (i = 0; i < MAX_B_CACHE; ++i) {
			BHIndex[i].uilru %= MAX_B_CACHE;
		}
		uiLRU = MAX_B_CACHE;
	}

	/* When hash value is not in the hash table add it, first clear
	 * the slot at the least recently used point.
	 */
	pBH = & BHIndex[iLooser];
	if ('\000' != pBH->acpath[0]) {
		pBH->pDBopen->close(pBH->pDBopen, 0);
		pBH->pDBopen = (DB *)0;
	}
	(void)strcpy(pBH->acpath, pcPath);
	if ((char *)0 != pcSubUnit) {
		*pcTail = '\000';
	}
	pBH->uilru = uiLRU;
	pBH->uihash = uiHash;
	if (0 != (iRet = db_create(&pBH->pDBopen, pDBEBtree, 0))) {
		fprintf(stderr, "%s: db_create: %s\n", progname, strerror(errno));
		fflush(stderr);
		return (DB *)0;
	}
	if (-1 != iPageSize) {
		pBH->pDBopen->set_pagesize(pBH->pDBopen, iPageSize);
	}
	if (0 != (iRet = pBH->pDBopen->open(pBH->pDBopen, pcPath, pcSubUnit, DB_BTREE, iFlags, 0666))) {
		pBH->pDBopen->err(pBH->pDBopen, iRet, "%s: db_open: %s(%s)", progname, pcPath, pcSubUnit ? pcSubUnit : "none");
		pBH->pDBopen->close(pBH->pDBopen, 0);
		return (DB *)0;
	}

	if ((DB *)0 == pBH->pDBopen) {
		pBH->acpath[0] = '\000';
	}
	return pBH->pDBopen;
}

/* close all the acitve databases					(ksb)
 * or sync them is flag is not zero.
 */
static void
FlushTrees(fFlag)
int fFlag;
{
	register int i;
	register BTREE_HASH *pBH;

	for (i = 0, pBH = & BHIndex[i]; i < MAX_B_CACHE; ++i, ++pBH) {
		if ('\000' == pBH->acpath[0] || (DB *)0 == pBH->pDBopen) {
			continue;
		}
		if (fFlag) {
			pBH->pDBopen->sync(pBH->pDBopen, 0);
			continue;
		}
		pBH->pDBopen->close(pBH->pDBopen, 0);
		pBH->acpath[0] = '\000';
		pBH->pDBopen = (DB *)0;
	}
}

/* create the unit file and initialize it to our liking			(ksb)
 * We don't put the ';'s back :-(
 */
static int
btree_init(char *pcPath, char *pcUnit, char *pcOpts)
{
	register DB *pDBCur;
	register int iWasSize;
	register char *pcSemi;

	/* look at pcOpts to set options.
	 */
	iWasSize = iPageSize;
	for (/*param*/; (char *)0 != pcOpts && '\000' != *pcOpts; pcOpts = pcSemi) {
		if ((char *)0 != (pcSemi = strchr(pcOpts, ';'))) {
			*pcSemi++ = '\000';
		}
		while (isspace(*pcOpts)) {
			++pcOpts;
		}
		if (0 == strncasecmp(pcOpts, "size=", 5)) {
			iPageSize = atoi(pcOpts+5);
			continue;
		}
		/* LLL more options */
	}
	pDBCur = FindTree(pcPath, pcUnit);
	iPageSize = iWasSize;
	return (DB *)0 == pDBCur ? -1 : 0;
}

static DBT DBTOld;
static char acEmpty[] = "";
%%
init "DBTOld.flags |= DB_DBT_REALLOC;"
%c

/* Update this unit's value for a specific name				(ksb)
 */
static int
btree_put(pcPath, pcUnit, pcName, pcValue, ppcOld)
char *pcPath, *pcUnit, *pcName, *pcValue, **ppcOld;
{
	register DB *pDBCur;
	register int iRet;
	auto DBT DBT1, DBT2;

	if ((DB *)0 == (pDBCur = FindTree(pcPath, pcUnit))) {
		*ppcOld = (char *)0;
		return -1;
	}
#if USE_STRINGS
	bzero(&DBT1, sizeof(DBT1));
	bzero(&DBT2, sizeof(DBT2));
#else
	(void)memset(&DBT1, 0, sizeof(DBT1));
	(void)memset(&DBT2, 0, sizeof(DBT2));
#endif
	DBT1.flags = DB_DBT_USERMEM;
	DBT1.data = pcName;
	DBT1.ulen = DBT1.size = strlen(DBT1.data)+1;
	if ((char **)0 != ppcOld) {
		if (0 != pDBCur->get(pDBCur, (void *)0, & DBT1, & DBTOld, 0)) {
			*ppcOld = (char *)0;
		} else if (0 == DBTOld.size) {
			*ppcOld = acEmpty;
		} else {
			*ppcOld = DBTOld.data;
		}
	}
	DBT2.flags = DB_DBT_USERMEM;
	DBT2.data = pcValue;
	DBT2.ulen = DBT2.size = (char *)0 == pcValue ? 0 : strlen(DBT2.data)+1;
	if (0 == (iRet = pDBCur->put(pDBCur, (void *)0, & DBT1, & DBT2, 0))) {
		return 0;
	}
	return iRet;
}

/* Read this unit's value for a name					(ksb)
 */
static int
btree_get(pcPath, pcUnit, pcName, ppcValue)
char *pcPath, *pcUnit, *pcName, **ppcValue;
{
	register DB *pDBCur;
	register int iRet;
	auto DBT DBT1;

	if ((DB *)0 == (pDBCur = FindTree(pcPath, pcUnit))) {
		*ppcValue = (char *)0;
		return -1;
	}
#if USE_STRINGS
	bzero(&DBT1, sizeof(DBT1));
#else
	(void)memset(&DBT1, 0, sizeof(DBT1));
#endif
	DBT1.flags = DB_DBT_USERMEM;
	DBT1.data = pcName;
	DBT1.ulen = DBT1.size = strlen(DBT1.data)+1;
	if (0 == (iRet = pDBCur->get(pDBCur, (void *)0, & DBT1, & DBTOld, 0))) {
		if (0 == DBTOld.size) {
			*ppcValue = acEmpty;
		} else {
			*ppcValue = DBTOld.data;
		}
		return 0;
	}
	*ppcValue = (char *)0;
	return iRet;
}

/* init the search buffers we must do once at startup			(ksb)
 */
%%
init 3 "btree_sinit();"
%c
static PPM_BUF PPMSIndex, PPMSStrings;

static void
btree_sinit()
{
	util_ppm_init(& PPMSIndex, sizeof(char *), 1024);
	util_ppm_init(& PPMSStrings, sizeof(char), 20480);
#if USE_STRINGS
	bzero(&DBTKey, sizeof(DBTKey));
	bzero(&DBTSData, sizeof(DBTSData));
#else
	(void)memset(&DBTKey, 0, sizeof(DBTKey));
	(void)memset(&DBTSData, 0, sizeof(DBTSData));
#endif
	DBTKey.flags |= DB_DBT_REALLOC;
	DBTSData.flags |= DB_DBT_REALLOC;
}

/* Search for a NVP, return a list of those that matched		(ksb)
 * When pcMatch is:
 * 	(char *)0	return all names (same as "")
 *	"="		return (all) names and values
 *	"prefix"	return names for all those matching "prefix"
 *	"prefix="	return names for all those matching "prefix" + values
 * Find the tree, setup a match criteria, set a cursor,
 * loop the keys and build an index of results
 * return the index, close the cursor
 */
static int
btree_search(pcPath, pcUnit, pcMatch, pppcFound)
char *pcPath, *pcUnit, *pcMatch, ***pppcFound;
{
	register int i, iMem, iIndex, iLen;
	register char **ppcIndex, *pcMem;
	register DB *pDBCur;
	register char *pcMatchEq;
	register int iRet;
	auto DBC *pDBCScan;

	if ((DB *)0 == (pDBCur = FindTree(pcPath, pcUnit))) {
		*pppcFound = (char **)0;
		return -1;
	}
	if ((char *)0 == pcMatch) {
		pcMatch = acEmpty;
		pcMatchEq = (char *)0;
	} else if ((char *)0 != (pcMatchEq = strchr(pcMatch, '='))) {
		*pcMatchEq = '\000';
	}
	if (0 == (iLen = strlen(pcMatch))) { /* review ZZZ */
		pcMatch = acEmpty;
	}

	if (0 != (iRet = pDBCur->cursor(pDBCur, (DB_TXN *)0, & pDBCScan, 0))) {
		return iRet;
	}
	iIndex = iMem = 0;
	while (0 == (iRet = pDBCScan->c_get(pDBCScan, & DBTKey, & DBTSData, DB_NEXT))) {
		register int fInclude;

		if (acEmpty != pcMatch && 0 != strncmp(pcMatch, DBTKey.data, iLen)) {
			continue;
		}
		i = iMem + DBTKey.size;
		fInclude = (char *)0 != pcMatchEq && 0 != DBTSData.size && (char *)0 != DBTSData.data;
		if (fInclude) {
			i += DBTSData.size;
		}
		ppcIndex = (char **)util_ppm_size(& PPMSIndex, iIndex+2);
		pcMem = (char *)util_ppm_size(& PPMSStrings, i+2);
		ppcIndex[iIndex++] = & pcMem[iMem];
		strcpy(pcMem+iMem, DBTKey.data);
		iMem += DBTKey.size;
		if (fInclude) {
			if ('\000' != pcMem[iMem-1]) {
				pcMem[iMem++] = '=';
			} else {
				pcMem[iMem-1] = '=';
			}
			strcpy(pcMem+iMem, DBTSData.data);
			iMem += DBTSData.size;
		}
		if ('\000' != pcMem[iMem-1]) {
			pcMem[iMem++] = '\000';
		}
	}
	ppcIndex = (char **)util_ppm_size(& PPMSIndex, iIndex+1);
	ppcIndex[iIndex] = (char *)0;
	(void)pDBCScan->c_close(pDBCScan);

	if (DB_NOTFOUND == iRet) {
		*pppcFound = ppcIndex;
		iRet = 0;
	} else {
		*pppcFound = (char **)0;
	}
	if ((char *)0 != pcMatchEq) {
		*pcMatchEq = '=';
	}
	return 0;
}

/* info/status request							(ksb)
 * Also used as a control point al-la ioctl(2).
 */
static int
btree_info(pcPath, pcUnit, pcCmd, ppcOut)
char *pcPath, *pcUnit, *pcCmd, **ppcOut;
{
	auto char *pcDump;

	if ((char **)0 != ppcOut) {
		*ppcOut = (char *)0;
	} else {
		ppcOut = & pcDump;
	}
	if ((char *)0 == pcCmd || 0 == strcmp("version", pcCmd)) {
		*ppcOut = btree_rcsid;
	}
	return 0;
}

/* clear any cached data and maybe close open fds if you can		(ksb)
 */
static int
btree_uncache()
{
	FlushTrees(1);
	sync();
	return 0;
}

/* get ready to exit this program					(ksb)
 */
static int
btree_cleanup()
{
	FlushTrees(0);
	sync();
	return 0;
}

static UNIT_CALLS UCBtree = {
	"btree", 2,
	btree_init,
	btree_put,
	btree_get,
	btree_search,
	btree_info,
	btree_uncache,
	btree_cleanup
};
%%

after {
	named "btree_after"
	update "%n();"
}

%c
/* put us in the correct place to run					(ksb)
 * open the master index's
 */
static void
btree_after()
{
	register int i;

	if (0 != chdir(pcRoot)) {
		fprintf(stderr, "%s: %s: chdir: %s: %s\n", progname, btree_progname, pcRoot, strerror(errno));
		exit(1);
	}

	if (0 != (i = db_env_create(& pDBEBtree, 0))) {
		if (DB_RUNRECOVERY == i) {
			fprintf(stderr, "%s: %s: db_env_create: %s: please run recovery procedure before restart\n", progname, btree_progname, pcRoot);
		} else {
			fprintf(stderr, "%s: %s: db_env_create: %s: %s\n", progname, btree_progname, pcRoot, strerror(errno));
		}
		exit(1);
	}
	/* tune us up for our login task, we call everything but:
	 *	set_feedback, set_recovery_init, set_verbose,
	 * XXX link the cachesize to a parameter or 2 (gb and b)
	 */
	db_env_set_mutexlocks(1);
	db_env_set_region_init(0);
	db_env_set_tas_spins(100);
	pDBEBtree->set_errpfx(pDBEBtree, progname);
	pDBEBtree->set_errfile(pDBEBtree, stderr);
	pDBEBtree->set_cachesize(pDBEBtree, 0, 128*1024*1024, 8);
	pDBEBtree->set_paniccall(pDBEBtree, BtreePanic);
	pDBEBtree->set_lk_max(pDBEBtree, MAX_B_CACHE+8);

	pDBEBtree->set_lg_dir(pDBEBtree, "log");
	pDBEBtree->set_tmp_dir(pDBEBtree, "tmp");
	i = pDBEBtree->open(pDBEBtree, ".", DB_CREATE|DB_INIT_LOCK|DB_INIT_LOG|DB_INIT_MPOOL|DB_INIT_TXN|fRecovery|DB_USE_ENVIRON|DB_USE_ENVIRON_ROOT, 0666);
	if (0 != i) {
		if (DB_RUNRECOVERY == i) {
			fprintf(stderr, "%s: db_env->open: %s: please rerun with -r, or -rr\n", progname, pcRoot);
		} else {
			fprintf(stderr, "%s: db_env->open: %s: %s\n", progname, pcRoot, strerror(errno));
		}
		exit(2);
	}

	/* set names, and clear tree cache
	 */
	for (i = 0; i < sizeof(BHIndex)/sizeof(BHIndex[0]); ++i) {
		BHIndex[i].uilru = 0;
		BHIndex[i].acpath[0] = '\000';
		BHIndex[i].pDBopen = (DB *)0;
	}
	uiLRU = 0U;
}
