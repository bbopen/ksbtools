#!mkcmd
# Remote Customer login services (RPC)					(ksb)
# This is the main program for the remote Customer login service
# RPC daemon.
from "<string.h>"
from "<unistd.h>"
from "<stdlib.h>"
from "<sys/types.h>"
from "<limits.h>"
from "<fcntl.h>"
from "<sys/stat.h>"
from "<dlfcn.h>"
from "<db.h>"

from '"machine.h"'
from '"main.h"'

require "shared.m" "util_errno.m" "std_help.m" "std_version.m"
require "xdr_Cstring.m" "xdr_Cargv.m"
require "rcls-inc,xdr,server.x"

%i
static char rcsid[] =
	"$Id: rcls.m,v 2.11 2000/06/14 21:54:44 ksb Exp $";

/* We must be able to open at least 2 trees for rename of a login to
 * work.
 */
#if !defined(RDB_CACHE_SIZE)
#define RDB_CACHE_SIZE		256		/* power of 2 please	*/
#endif

/* function to check the password for a Login, default to strcmp	(ksb)
 * but strcasecmp might be a better pick for some people
 */
#if !defined(RDB_PASS_CHECK)
#define RDB_PASS_CHECK	strcmp
#endif

/* the minimum length of a valid login name
 */
#if !defined(MIN_LOGIN_LEN)
#define MIN_LOGIN_LEN	3	/* for websso we don't allow short names */
#endif

static int argcSetup;	/* params from the commad line passed to setup	*/
static char **argvSetup;/* for each class we load, no defined meaning	*/
static char *apcFake[2];
%%

type "rpc_server" named "rclsprog_1" {
	key 1 {
		"RCLSPROG" "RCLSVERS"
	}
}

char* 'D' {
	named "pcClassDir"
	init '"/usr/local/lib/rcls"'
	param "dir"
	help "class modules location"
}

char* 'R' {
	named "pcRoot"
	init '"/var/rcls"'
	param "root"
	help "the top of the index heirarchy"
}

boolean 'r' {
	named "fRecovery"
	track "fGaveRecovery"
	update "%n = fGaveRecovery ? DB_RECOVER_FATAL : DB_RECOVER;"
	help "run recovery, or given more than once run recovery_fatal"
}

integer 'P' {
	named "iParallel"
	init "1"
	help "number of tasks to run in parallel"
	param "[jobs]"
	noparam "if (%ypi == (%N = getenv(\"PARALLEL\")) || '\\000' == *%N) {%N = \"3\";}"
	after "if (%n < 1) {%n = 1;}"
}

after {
}

list {
	param "class [args]"
	help "extra arguments to setup classes, separate them with --"
	ends
}

%c
/* Index routines (to find a Customer's login record			(ksb)
 * Each customer is found by looking for a btree in the current
 * directory named for the first letter inthe Cusomer's login,
 * in that tree we look for the _rest_ of the Customer's name
 * as a key.  the datum on that key is a record of the password
 * the number of protected NVPs, the protected NVP list, the
 * number of public NVPs and that list.
 * See TreeEncode/TreeDecode below.
 */

/* We can have up to RDB_CACHE_SIZE trees open.
 */
typedef struct BRnode {		/* See after_func for init code	 */
	char ackey[4];		/* name for this tree			*/
	unsigned int uilru;	/* least recently used counter		*/
	DB *pDBopen;
} BTREE_REC;
static char aacName[256][4];	/* name for this tree			*/
static BTREE_REC BRIndex[RDB_CACHE_SIZE];/* one for each open tree	*/
static unsigned int uiLRU;	/* global least recently used		*/
static int iMinAct, iMaxAct;	/* lowest and highest (+1) active tree	*/
static DB_ENV *pDBEGlobal;	/* upgrade to Berk DB 3.0.55		*/

static void
MyPanic(DB_ENV *pDBEWhich, int iErr)
{
	fprintf(stderr, "%s: database recovery needed %d\n", progname, iErr);
	/* XXX should syslog or labc here -- ksb */
	exit(125);
}

/* put us in the correct place to run					(ksb)
 * open the master index's
 */
void
after_func()
{
	register int i;

	if (0 != chdir(pcRoot)) {
		fprintf(stderr, "%s: chdir: %s: %s\n", progname, pcRoot, strerror(errno));
		exit(1);
	}

	if (0 != (i = db_env_create(& pDBEGlobal, 0))) {
		if (DB_RUNRECOVERY == i) {
			fprintf(stderr, "%s: db_env_create: %s: please run recovery procedure before restart\n", progname, pcRoot);
		} else {
			fprintf(stderr, "%s: db_env_create: %s: %s\n", progname, pcRoot, strerror(errno));
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
	pDBEGlobal->set_errpfx(pDBEGlobal, progname);
	pDBEGlobal->set_errfile(pDBEGlobal, stderr);
	pDBEGlobal->set_cachesize(pDBEGlobal, 0, 640*1024*1024, 10);
	pDBEGlobal->set_paniccall(pDBEGlobal, MyPanic);
	pDBEGlobal->set_lk_max(pDBEGlobal, 260);
	/* added for db-3.1 release
	 *	set_data_dir(pDBEGlobal, ".");
	 */
	pDBEGlobal->set_lg_dir(pDBEGlobal, "log");
	pDBEGlobal->set_tmp_dir(pDBEGlobal, "tmp");

	i = pDBEGlobal->open(pDBEGlobal, ".", DB_CREATE|DB_INIT_LOCK|DB_INIT_LOG|DB_INIT_MPOOL|DB_INIT_TXN|fRecovery|DB_USE_ENVIRON|DB_USE_ENVIRON_ROOT, 0666);
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
	for (i = 0; i < 256; ++i) {
		if (isalnum(i) && '/' != i && '.' != i) {
			aacName[i][0] = i;
			aacName[i][1] = '\000';
		} else {
			sprintf(aacName[i], "%02x", i);
		}
	}
	for (i = 0; i < sizeof(BRIndex)/sizeof(BRIndex[0]); ++i) {
		BRIndex[i].uilru = 0;
		BRIndex[i].ackey[0] = '\000';
		BRIndex[i].ackey[1] = '\000';
		BRIndex[i].pDBopen = (DB *)0;
	}
	uiLRU = 0U;
	iMinAct = iMaxAct = 0;
}


/* Find the index btree for the given login name, open it and		(ksb)
 * the DB is a BTREE named for the first character (1) with subtrees
 * for each second character (2).  The keys are characters 3..N\000
 * I think login names are at least 3 characters. (We could accept 2\0).
 */
static DB *
FindIdx(pcLogin)
char *pcLogin;
{
	register int i, iLooser, iC1, iC2, iK1, iRet;
	register BTREE_REC *pBR;
	register unsigned int uiAge;

	iLooser = -1;
	iC1 = pcLogin[0], iC2 = pcLogin[1];
	for (i = iMinAct, pBR = & BRIndex[i]; i < iMaxAct; ++i, ++pBR) {
		if ('\000' == (iK1 = pBR->ackey[0]))
			continue;
		if (-1 == iLooser) {
			iLooser = i;
			uiAge = pBR->uilru;
		} else if (pBR->uilru < uiAge) {
			iLooser = i;
			uiAge = pBR->uilru;
		}
		if (iC1 != iK1 || iC2 != pBR->ackey[1]) {
			continue;
		}
		/* found it open already, update last use to current
		 */
		pBR->uilru = uiLRU;
		return pBR->pDBopen;
	}

	/* Not found, make a slot.  If we have space open another
	 * if we don't then close the oldest one.
	 */
	if (iMinAct > 0) {
		iLooser = --iMinAct;
	} else if (iMaxAct < sizeof(BRIndex)/sizeof(BRIndex[0])) {
		iLooser = iMaxAct++;
	} else if (-1 == iLooser) {
		/* how did this happen? */
		return (DB *)0;
	} else {
		pBR = & BRIndex[iLooser];
		pBR->pDBopen->close(pBR->pDBopen, 0);
		pBR->pDBopen = (DB *)0;		/* redundant */
	}

	/* Update least recently used counter, when we would wrap back to
	 * 0 we wrap to RDB_CACHE_SIZE and clear the high bits on the
	 * existing LRU table.
	 */
	if (0U == ++uiLRU) {
		for (i = 0; i < sizeof(BRIndex)/sizeof(BRIndex[0]); ++i) {
			BRIndex[i].uilru %= sizeof(BRIndex)/sizeof(BRIndex[0]);
		}
		uiLRU = sizeof(BRIndex)/sizeof(BRIndex[0]);
	}

	/* make it so, dude
	 */
	pBR = & BRIndex[iLooser];
	pBR->ackey[0] = iC1;
	pBR->ackey[1] = iC2;
	pBR->uilru = uiLRU;
	if (0 != (iRet = db_create(&pBR->pDBopen, pDBEGlobal, 0))) {
		fprintf(stderr, "%s: db_create: %s\n", progname, strerror(errno));
		fflush(stderr);
		return (DB *)0;
	}
	if (0 != (iRet = pBR->pDBopen->open(pBR->pDBopen, aacName[iC1], aacName[iC2], DB_BTREE, DB_CREATE, 0666))) {
		pBR->pDBopen->err(pBR->pDBopen, iRet, "%s: db_open: %s(%s)", progname, aacName[iC1], aacName[iC2]);
		pBR->pDBopen->close(pBR->pDBopen, 0);
		return (DB *)0;
	}

	if ((DB *)0 == pBR->pDBopen) {
		pBR->ackey[0] = '\000';
	}
	return pBR->pDBopen;
}

/* close all the acitve databases					(ksb)
 * or sync them is flag is not zero.
 */
static void
FlushIdx(fFlag)
int fFlag;
{
	register int i;
	register BTREE_REC *pBR;

	for (i = iMinAct, pBR = & BRIndex[i]; i < iMaxAct; ++i, ++pBR) {
		if ('\000' == pBR->ackey[0] || (DB *)0 == pBR->pDBopen) {
			continue;
		}
		if (fFlag) {
			pBR->pDBopen->sync(pBR->pDBopen, 0);
			continue;
		}
		pBR->pDBopen->close(pBR->pDBopen, 0);
		pBR->ackey[0] = '\000';
		pBR->pDBopen = (DB *)0;
	}
	iMinAct = iMaxAct = 0;
}


/* on a SIGTERM cleanup and exit nicely					(ksb)
 */
static void
ShutDown()
{
	svc_unregister(RCLSPROG, RCLSVERS);
	FlushIdx(0);
	sync();
	exit(0);
}

/* Register a class module for global reference				(ksb)
 * Give me an entry and I'll record it, give be (void *)0 and I'll find
 * it.  When we are recording we strdup any dynamic names.
 *
 * We are not very clever here -- just linear scans. -- ksb
 */
typedef struct MTnode {
	char *pcname;
	void *pvmodule;
} MOD_ENTRY;
static MOD_ENTRY *pMTAll = (MOD_ENTRY *)0;
static int iMods = 0, iMaxMods = 0;
void *
class_module(pcName, pDLModule)
char *pcName;
void *pDLModule;
{
	register int i;

	for (i = 0; i < iMods; ++i) {
		if (0 == strcmp(pMTAll[i].pcname, pcName))
			return pMTAll[i].pvmodule;
	}
	if ((void *)0 == pDLModule) {
		return pDLModule;
	}

	if (iMaxMods == i) {
		register MOD_ENTRY *pMTMem;
		iMaxMods += 32;
		if ((MOD_ENTRY *)0 == pMTAll)
			pMTMem = calloc(iMaxMods, sizeof(MOD_ENTRY));
		else
			pMTMem = realloc((void *)pMTAll, iMaxMods*sizeof(MOD_ENTRY));
		if ((MOD_ENTRY *)0 == pMTMem) {
			return (MOD_ENTRY *)0;
		}
		pMTAll = pMTMem;
	}
	pMTAll[iMods].pcname = strdup(pcName);
	pMTAll[iMods++].pvmodule = pDLModule;

	return pDLModule;
}

static char acSetup[] = "setup";

/* return an entry from the named module, or (void *)0			(ksb)
 */
void *
class_entry(pcModule, pcEntry)
char *pcModule, *pcEntry;
{
	register void *pvModule;
	register int (*pfiSetup)();
	auto char acModPath[MAXPATHLEN+4];

	pvModule = class_module(pcModule, (void *)0);
	if ((void *)0 == pvModule) {
		(void)sprintf(acModPath, "%s/%s.so", pcClassDir, pcModule);
		pvModule = dlopen(acModPath, RCLS_RTLD_OPTS);
		if ((void *)0 == pvModule) {
			return (void *)0;
		}
		(void)class_module(pcModule, pvModule);
		pfiSetup = (int (*)())dlsym(pvModule, acSetup);
		/* LLL we don't let this fail ?
		 */
		if ((int (*)())0 != pfiSetup) {
			apcFake[0] = pcModule;
			pfiSetup(argcSetup, argvSetup, pcModule);
		}
	}
	return (void *)dlsym(pvModule, pcEntry);
}

/* load the class before we even take the first request			(ksb)
 */
static void
PreLoad(ppcArgs)
char **ppcArgs;
{
	register char *pcClass;
	register int i;

	if ((char **)0 == ppcArgs || (char *)0 == (pcClass = *ppcArgs)) {
		return;
	}
	i = 0;
	argvSetup = ppcArgs;
	while ((char *)0 != *ppcArgs) {
		++ppcArgs, ++i;
	}
	argcSetup = i;
	printf("%s: Preload %s(%d)\n", progname, pcClass, i);
	if ((void *)0 == class_entry(pcClass, acSetup)) {
		fprintf(stderr, "%s: %s: %s entry missing -- preload failed\n", progname, pcClass, acSetup);
		exit(10);
	}
}


/* Run the RPC service, when it's done flush the index and exit		(ksb)
 * Accept params "sso -b thing -A -C file.cf -- employee -FR -q 2"
 * to mean PreLoad the class "sso" with options -b, -A, -C and class
 * employee with options -F, -R, -q).
 */
int
list_func(argc, argv)
int argc;
char **argv;
{
	register char **ppcDash, *pcParam;

	for (ppcDash = argv; (char *)0 != (pcParam = *ppcDash); ++ppcDash) {
		if ('-' == pcParam[0] && '-' == pcParam[1] && '\000' == pcParam[2]) {
			*ppcDash = (char *)0;
			PreLoad(argv);
			*ppcDash = pcParam;
			argv = ppcDash+1;
		}
	}
	if ((char *)0 != *argv) {
		PreLoad(argv);
		printf("%s: preloads done\n", progname);
	}

	/* pass nothing useful to non-preloaded classes
	 */
	argcSetup = 1;
	apcFake[0] = progname;
	apcFake[1] = (char *)0;
	argvSetup = apcFake;

	(void)signal(SIGTERM, (void *)ShutDown);
	u_rpc_reg(RPC_ANYSOCK);
	svc_run();
	/*NOTREACHED*/
	exit(0);
}



/* Encode the data for insertion into some index btree			(ksb)
 *
 * "cnt" means "count of the" here.
 * This routine holds the buffer space, don't mess with it.
 *	password '\000' cntPub '\000' cntPro '\000'
 *	  pub1 '\000' pub2 '\000' ... pubM '\000'
 *	  prot1 '\000' pro2 '\000' ...  protN '\000' '\000'
 */
static void
TreeEncode(pDBT, pcPassword, ppcProtect, ppcPublic)
DBT *pDBT;
char *pcPassword, **ppcProtect, **ppcPublic;
{
	static char *pcMem;
	static int iMem = 0;
	register char **ppc, *pcScan;
	register int iStrSpace, iCntPro, iCntPub;
	auto char acCntPro[32], acCntPub[32];

	if ((char *)0 == pcPassword) {
		pcPassword = "";
	}

	iCntPro = iCntPub = 0;
	iStrSpace = strlen(pcPassword);
	if ((char **)0 != (ppc = ppcPublic)) {
		while ((char *)0 != (pcScan = *ppc++)) {
			iStrSpace += strlen(pcScan);
			++iCntPub;
		}
	}
	if ((char **)0 != (ppc = ppcProtect)) {
		while ((char *)0 != (pcScan = *ppc++)) {
			iStrSpace += strlen(pcScan);
			++iCntPro;
		}
	}
	sprintf(acCntPub, "%d", iCntPub);
	sprintf(acCntPro, "%d", iCntPro);
	iStrSpace += strlen(acCntPub) + strlen(acCntPro);

	/* Finish by adding space for  nulls
	 *	passwd+cnt+cnt + pro's + pubs + EOD
	 */
	iStrSpace += 3 + iCntPub + iCntPro +1;

	/* do we have enough space?
	 */
	if (iMem < iStrSpace) {
		iStrSpace |= 1023;
		++iStrSpace;
		if (0 == iMem) {
			pcMem = malloc(iStrSpace);
		} else {
			pcMem = realloc((void *)pcMem, iStrSpace);
		}
		if ((char *)0 == pcMem) {
			pDBT->data = "foobie~out~of~mem\000" "0\000" "0\000\000";
			pDBT->size = strlen(pDBT->data)+6;
			pcMem = (char *)0;
			iMem = 0;
			return;
		}
		iMem = iStrSpace;
	}
	/* Build the record
	 * pass pub pro pub-list pro-list '\000'
	 */
	pcScan = pcMem;
	(void)strcpy(pcScan, pcPassword);
	pcScan += strlen(pcScan)+1;
	(void)strcpy(pcScan, acCntPub);
	pcScan += strlen(pcScan)+1;
	(void)strcpy(pcScan, acCntPro);
	pcScan += strlen(pcScan)+1;
	for (ppc = ppcPublic; iCntPub-- > 0; ++ppc) {
		(void)strcpy(pcScan, *ppc);
		pcScan += strlen(pcScan)+1;
	}
	for (ppc = ppcProtect; iCntPro-- > 0; ++ppc) {
		(void)strcpy(pcScan, *ppc);
		pcScan += strlen(pcScan)+1;
	}
	*pcScan++ = '\000';

	pDBT->flags = DB_DBT_USERMEM;
	pDBT->data = pcMem;
	pDBT->size = pcScan - pcMem;
}


/* Create a new login in the index tables				(ksb)
 * We are given the login/password/protected-attrs/public-attrs and class.
 * If login doesn't exist we filter the attrs through the class method
 * (which might add some to the lists) then build the encoded blob and
 * install it in the index.  Then we call the class's commit if it has one.
 * 0 OK
 * 1 duplicate login name
 * 2 btree level failure
 * 3 input params are not good (null poointer, short login name, dup)
 */
create_res *
loginrcreate_1(argp, rqstp)
create_param *argp;
struct svc_req *rqstp;
{
	static create_res  result;
	register DB *pDBCur;
	auto DBT DBT1, DBT2;
	auto DB_TXN *pTXN;
	register int (*pfiInit)();
	register char *pcClass;

	/* check input params, 3 means we don't love them
	 */
	result.iret = 3;
	result.ppcprotect = (char **)0;
	result.ppcpublic = (char **)0;
	if ((char *)0 == argp->pclogin || (char *)0 == argp->pcpassword || strlen(argp->pclogin) < MIN_LOGIN_LEN) {
		return &result;
	}
	if ((DB *)0 == (pDBCur = FindIdx(argp->pclogin))) {
		result.iret = 2;
		return &result;
	}

#if USE_STRINGS
	bzero(&DBT1, sizeof(DBT1));
	bzero(&DBT2, sizeof(DBT2));
#else
	(void)memset(&DBT1, 0, sizeof(DBT1));
	(void)memset(&DBT2, 0, sizeof(DBT2));
#endif
	DBT1.flags = DB_DBT_USERMEM;
	DBT1.data = argp->pclogin+2;
	DBT1.ulen = DBT1.size = strlen(DBT1.data)+1;
	if (0 != txn_begin(pDBEGlobal, (DB_TXN *)0, & pTXN, DB_TXN_SYNC)) {
		result.iret = -1;
		return & result;
	}
	result.iret = pDBCur->get(pDBCur, pTXN, & DBT1, & DBT2, 0);
	switch (result.iret) {
	case 0:	/* already exists */
		result.iret = 1;
		/* fall through */
	default: /* error */
		txn_abort(pTXN);
		return &result;
	case DB_NOTFOUND:	/* not found */
		break;
	}
	pcClass = ((char *)0 == argp->pcclass) ? "default" : argp->pcclass;
	pfiInit = class_entry(pcClass, "init");
	if ((int (*)())0 != pfiInit) {
		result.iret = pfiInit(argp, &result.ppcprotect, &result.ppcpublic);
	} else {
		result.ppcprotect = argp->ppcprotect;
		result.ppcpublic  = argp->ppcpublic;
	}
#if RCLS_TRACE
	printf("new \"%s\", class %s: init %d\n", argp->pclogin, pcClass, result.iret);
#endif

	if (0 == result.iret) {
		TreeEncode(& DBT2, argp->pcpassword, result.ppcprotect, result.ppcpublic);
		result.iret = pDBCur->put(pDBCur, pTXN, & DBT1, & DBT2, DB_NOOVERWRITE);
	}

	if (0 == result.iret) {
		result.iret = txn_commit(pTXN, DB_TXN_SYNC);
	} else {
		txn_abort(pTXN);
		if (DB_KEYEXIST == result.iret) {
			result.iret = 1;
		}
	}
	return &result;
}

/* Unpack the disk format for the Customer index attributes		(ksb)
 * See Encode for diagram (above), roughly:
 *	<pass> <cntPub> <cntPro> <pub-list> <pro-list>
 */
static int
TreeDecode(pcPacked, ppcPassword, pppcProtect, pppcPublic)
char *pcPacked, **ppcPassword, ***pppcProtect, ***pppcPublic;
{
	static char **ppcVec = (char **)0;
	static int iMaxVec = 0;
	register int iCntPro, iCntPub, i;
	register char **ppc;

	if ((char **)0 != ppcPassword) {
		*ppcPassword = pcPacked;
	}
	pcPacked += strlen(pcPacked)+1;
	iCntPub = atoi(pcPacked);
	pcPacked += strlen(pcPacked)+1;
	iCntPro = atoi(pcPacked);
	pcPacked += strlen(pcPacked)+1;
	if (iMaxVec < iCntPub+iCntPro+2) {
		register int iTemp;
		register char **ppcTemp;
		iTemp = (iCntPub+iCntPro+1)|31;
		++iTemp;
		if (0 == iMaxVec)
			ppcTemp = (char **)malloc(iTemp*sizeof(char *));
		else
			ppcTemp = (char **)realloc((void *)ppcVec, iTemp*sizeof(char *));
		if ((char **)0 == ppcTemp)
			return 1;
		ppcVec = ppcTemp;
		iMaxVec = iTemp;
	}
	ppc = ppcVec;
	for (i = 0; i < iCntPub; ++i) {
		*ppc++ = pcPacked;
		pcPacked += strlen(pcPacked)+1;
	}
	if ((char ***)0 != pppcPublic) {
		*pppcPublic = ppcVec;
		*ppc++ = (char *)0;
	}
	if ((char ***)0 == pppcProtect) {
		return 0;
	}
	*pppcProtect = ppc;
	for (i = 0; i < iCntPro; ++i) {
		*ppc++ = pcPacked;
		pcPacked += strlen(pcPacked)+1;
	}
	*ppc = (char *)0;
	return 0;
}


/* Try to establish creds as the given login/password pair		(ksb)
 * This is cool because a failed login still gets you the public
 * attributes of the login.
 */
login_res *
loginropen_1(argp, rqstp)
login_param *argp;
struct svc_req *rqstp;
{
	static login_res  result;
	register DB *pDBCur;
	auto char *pcCheckPass;
	auto DBT DBT1, DBT2;

	result.iret = 2;
	result.ppcprotect = (char **)0;
	result.ppcpublic = (char **)0;

	if ((DB *)0 == (pDBCur = FindIdx(argp->pclogin))) {
		result.iret = 2;
		return &result;
	}

#if USE_STRINGS
	bzero(&DBT1, sizeof(DBT1));
	bzero(&DBT2, sizeof(DBT2));
#else
	(void)memset(&DBT1, 0, sizeof(DBT1));
	(void)memset(&DBT2, 0, sizeof(DBT2));
#endif
	DBT1.flags = DB_DBT_USERMEM;
	DBT1.data = argp->pclogin+2;
	DBT1.size = strlen(DBT1.data)+1;
	result.iret = pDBCur->get(pDBCur, (DB_TXN *)0, & DBT1, & DBT2, 0);
	switch (result.iret) {
	case 0:	/* found */
		break;
	case DB_NOTFOUND:	/* not found */
	default: /* error */
		return &result;
	}

	if (0 == RDB_PASS_CHECK(DBT2.data, argp->pcpassword)) {
		TreeDecode(DBT2.data, &pcCheckPass, &result.ppcprotect, &result.ppcpublic);
	} else {
		TreeDecode(DBT2.data, &pcCheckPass, (char ***)0, &result.ppcpublic);
	}
	return &result;
}

/* update the password in the encoded record				(ksb)
 * If there is not enough space in the existing buffer make a new one
 * and copy the rest to it (everything after the password).
 *	password \0 cntA \0 cntB \0 strings \0
 * then move the data pointer to the new record.
 *
 * Return the new data, or (char *)0 when malloc fails.
 */
static char *
TreeChPass(pDBT, pcNewPassword)
DBT *pDBT;
char *pcNewPassword;
{
	static char *pcMem;
	static int iMax = 0;
	register int iOldLen, iNewLen, i;

	if ((char *)0 == pcNewPassword) {
		return pDBT->data;
	}
	if ((iOldLen = strlen(pDBT->data)) < (iNewLen = strlen(pcNewPassword))) {
		i = pDBT->size - iOldLen + iNewLen;
		if (i > iMax) {
			iMax = (i | 511)+1;
			if ((char *)0 == pcMem) {
				pcMem = malloc(iMax);
			} else {
				pcMem = realloc((void *)pcMem, iMax);
			}
		}
		if ((char *)0 == pcMem) {
			return (char *)0;
		}
#if USE_STRINGS
		bcopy(pDBT->data+iOldLen, pcMem+iNewLen, pDBT->size-iOldLen);
#else
		memcpy(pcMem+iNewLen, pDBT->data+iOldLen, pDBT->size-iOldLen);
#endif
		pDBT->data = pcMem;
		pDBT->size = i;
		iOldLen = iNewLen;
	} else {
		/* we know we have enough space now, might be too much
		 * so we just move beyond the start a little.
		 */
		iOldLen -= iNewLen;
		pDBT->data += iOldLen;
		pDBT->size -= iOldLen;
	}
	return strcpy(pDBT->data, pcNewPassword);
}

/* Rename a login, or change the password, not implmented yet.		(ksb)
 * If the password is (char *)0 we can change the password to anything we
 * want.  If the password matches we can change the password.
 * We can rename the login now (couldn't before).
 */
rename_res *
loginrrename_1(argp, rqstp)
rename_param *argp;
struct svc_req *rqstp;
{
	static rename_res  result;
	register DB *pDBCur, *pDBNew;
	register char *pcNewLogin;
	auto DB_TXN *pTXN;
	auto DBT DBT1, DBT2, DBTNew;

	if ((char *)0 == argp->pclogin) {
		result.iret = 2;
		return & result;
	}

	/* setup for any login name changes, and cancel same_name=same_name
	 */
	if ((char *)0 != (pcNewLogin = argp->pcnewlogin)) {
		if (strlen(pcNewLogin) < MIN_LOGIN_LEN) {
			result.iret = 2;
			return &result;
		}
		if (0 == strcmp(argp->pclogin, pcNewLogin)) {
			pcNewLogin = (char *)0;
		} else if ((DB *)0 == (pDBNew = FindIdx(pcNewLogin))) {
			result.iret = 2;
			return &result;
		}
	}
	if ((DB *)0 == (pDBCur = FindIdx(argp->pclogin))) {
		result.iret = 2;
		return &result;
	}
#if USE_STRINGS
	bzero(&DBT1, sizeof(DBT1));
	bzero(&DBT2, sizeof(DBT2));
	bzero(&DBTNew, sizeof(DBTNew));
#else
	(void)memset(&DBT1, 0, sizeof(DBT1));
	(void)memset(&DBT2, 0, sizeof(DBT2));
	(void)memset(&DBTNew, 0, sizeof(DBTNew));
#endif
	DBT1.flags = DB_DBT_USERMEM;
	DBT1.data = argp->pclogin+2;
	DBT1.size = strlen(DBT1.data)+1;
	if (0 != txn_begin(pDBEGlobal, (DB_TXN *)0, & pTXN, DB_TXN_SYNC)) {
		result.iret = -1;
		return & result;
	}
	result.iret = pDBCur->get(pDBCur, pTXN, & DBT1, & DBT2, DB_RMW);
	switch (result.iret) {
	case 0:	/* found */
		break;
	case DB_NOTFOUND:	/* not found */
	default: /* error */
		txn_abort(pTXN);
		return &result;
	}
	if ((char *)0 != argp->pcpassword && 0 != RDB_PASS_CHECK(DBT2.data, argp->pcpassword)) {
		result.iret = 1;
		txn_abort(pTXN);
		return &result;
	}

	/* Update the record and put it back
	 * if there is space for the new password just overwrite it,
	 * else do it the harder way.  We know about encodings.
	 */
	if ((char *)0 != argp->pcnewpassword && (char *)0 == TreeChPass(& DBT2, argp->pcnewpassword)) {
		result.iret = -1;
		txn_abort(pTXN);
		return &result;
	}
	if ((char *)0 == pcNewLogin) {
		result.iret = pDBCur->put(pDBCur, pTXN, & DBT1, & DBT2, 0);
	} else {
		DBTNew.flags = DB_DBT_USERMEM;
		DBTNew.data = argp->pcnewlogin+2;
		DBTNew.size = strlen(DBTNew.data)+1;
		result.iret = pDBNew->put(pDBNew, pTXN, & DBTNew, & DBT2, DB_NOOVERWRITE);
		if (0 == result.iret) {
			result.iret = pDBCur->del(pDBCur, pTXN, & DBT1, 0);
		}
	}

	if (0 == result.iret) {
		result.iret = txn_commit(pTXN, DB_TXN_SYNC);
	} else {
		(void)txn_abort(pTXN);
	}
	return &result;
}

/* local scratch for arrtubute update/delete				(ksb)
 *
 * Don't call MergeAttrs twice in the same RPC call without allocating
 * a "mattr_buf" for each call (2..N) and passing it's address.  If you
 * are not a toplevel call you _must_ have a static mattr_buf of you own.
 */
typedef struct MAnode {
	int imax;
	char **ppc;
} mattr_buf;

/* Merge the two attribute lists					(ksb)
 * None in the current, then use the add list as-is.
 * None in the add list, return happy;
 * Otherwise we build a conservative index structure (we won't copy
 * the strings) and build the ressult in that.  pMA has that in it.
 *
 * Bugs: if the ppcAdd list has more than one value for an attribute
 *	we install more than one value in the list.
 */
void
MergeAttrs(pMA, pppcCur, ppcAdd)
mattr_buf *pMA;
char ***pppcCur, **ppcAdd;
{
	register char **ppcList, **ppcNew, *pcOld;
	register int iCur, iAdd, i, j;
	static mattr_buf MACatch;

	if ((mattr_buf *)0 == pMA) {
		pMA = & MACatch;
	}
	if ((char ***)0 == pppcCur) {
		pppcCur = & pMA->ppc;
	}
	if ((char **)0 == (ppcList = *pppcCur) || (char *)0 == *ppcList) {
		if ((char **)0 != ppcAdd) {
			*pppcCur = ppcAdd;
		}
		return;
	}
	if ((char **)0 == ppcAdd || (char *)0 == *ppcAdd) {
		return;
	}
	for (iAdd = 0; (char *)0 != ppcAdd[iAdd]; ++iAdd) {
		/* count them */
	}
	for (iCur = 0; (char *)0 != ppcList[iCur]; ++iCur) {
		/* count them */
	}
	/* We need at most a vector of <cur + add + sentinal-value>:
	 * so build a scratch index, or enlarge it when we need more.
	 */
	i = iCur+iAdd+1;
	if (i > pMA->imax) {
		pMA->imax = (i|63)+1;
		if ((char **)0 == pMA->ppc) {
			pMA->ppc = malloc(pMA->imax * sizeof(char *));
		} else {
			pMA->ppc = realloc((void *)pMA->ppc, pMA->imax * sizeof(char *));
		}
		/* XXX we don't check the allocation here
		 */
	}
	ppcNew = pMA->ppc;

	/* Transfer the add list to end of return vector, so we can
	 * remove the ones we replace in the original list
	 */
	for (j = 0; j < iAdd; ++j) {
		ppcNew[iCur+j] = ppcAdd[j];
	}
	ppcNew[iCur+j] = (char *)0;

	/* copy in current list, overwrite with new, when we find commons
	 */
	for (i = 0; (char *)0 != (pcOld = *ppcList++); ++i) {
		register int iLen;
		register char *pcEq, *pcNew;

		if ((char *)0 != (pcEq = strchr(pcOld, '='))) {
			iLen = pcEq - pcOld;
		} else {
			iLen = strlen(pcOld);
			pcEq = pcOld + iLen;
		}
		for (j = iCur; (char *)0 != (pcNew = ppcNew[j]); ++j) {
			if (0 != strncmp(pcNew, pcOld, iLen)) {
				continue;
			}
			switch (pcNew[iLen]) {
			case '\000':
			case '=':
				break;
			default:
				continue;
			}
			break;
		}
		if ((char *)0 == pcNew) {
			ppcNew[i] = pcOld;
			continue;
		}
		ppcNew[i] = pcNew;
		/* consume the value by moving the last into it's slot
		 * and null'ing the last.  This works even if we are the
		 * last, BTW.
		 */
		--iAdd;
		ppcNew[j] = ppcNew[iCur+iAdd];
		ppcNew[iCur+iAdd] = (char *)0;
	}
	/* Add the remaining ones from the add list by not doing anything
	 * {the assignment below is redunant}.
	 */
	ppcNew[iCur+iAdd] = (char *)0;

	/* if you want the sorted define a function to do that and
	 * define the macro "MUST_SORT" to be that function's name.
	 */
#if defined(MUST_SORT)
	qsort((void *)ppcNew, iAdd, sizeof(char *), MUST_SORT);
#endif
	*pppcCur = ppcNew;
}

/* Update the attributes on a login, not implemented yet.		(ksb)
 * An update on a existing attribute replaces the value, new attributes
 * are appended (maybe sorted or not).  Existing attributes that are
 * not mentioned are not impacted.
 *
 * If the password is given and doesn't match we fail (return 1).
 *
 * If the password is (char *)0 only the public attributes may be updated,
 * existing private attributes are never affected by this.
 */
create_res *
loginrupdate_1(argp, rqstp)
update_param *argp;
struct svc_req *rqstp;
{
	static create_res  result;
	register DB *pDBCur;
	auto char *pcCurPass;
	auto DBT DBT1, DBT2;
	auto DB_TXN *pTXN;

	/* check perm on implied operation
	 */
	if ((char *)0 == argp->pcpassword && (char **)0 != argp->ppcprotect) {
		result.iret = 1;
		return &result;
	}

	if ((DB *)0 == (pDBCur = FindIdx(argp->pclogin))) {
		result.iret = 2;
		return &result;
	}

#if USE_STRINGS
	bzero(&DBT1, sizeof(DBT1));
	bzero(&DBT2, sizeof(DBT2));
#else
	(void)memset(&DBT1, 0, sizeof(DBT1));
	(void)memset(&DBT2, 0, sizeof(DBT2));
#endif
	DBT1.flags = DB_DBT_USERMEM;
	DBT1.data = argp->pclogin+2;
	DBT1.size = strlen(DBT1.data)+1;
	if (0 != txn_begin(pDBEGlobal, (DB_TXN *)0, & pTXN, DB_TXN_SYNC)) {
		result.iret = -1;
		return & result;
	}
	result.iret = pDBCur->get(pDBCur, pTXN, & DBT1, & DBT2, DB_RMW);
	switch (result.iret) {
	case 0:	/* found */
		break;
	case DB_NOTFOUND:	/* not found */
	default: /* error */
		txn_abort(pTXN);
		return &result;
	}
	if ((char *)0 != argp->pcpassword && 0 != RDB_PASS_CHECK(DBT2.data, argp->pcpassword)) {
		result.iret = 1;
		txn_abort(pTXN);
		return &result;
	}
	/* update record any put it back
	 */
	TreeDecode(DBT2.data, &pcCurPass, &result.ppcprotect, &result.ppcpublic);
	if ((char **)0 != argp->ppcprotect) {
		static mattr_buf MAPro;
		MergeAttrs(&MAPro, &result.ppcprotect, argp->ppcprotect);
	}
	if ((char **)0 != argp->ppcpublic) {
		static mattr_buf MAPub;
		MergeAttrs(&MAPub, &result.ppcpublic, argp->ppcpublic);
	}
	TreeEncode(& DBT2, pcCurPass, result.ppcprotect, result.ppcpublic);
	if ((char *)0 == argp->pcpassword) {
		result.ppcprotect = (char **)0;
	}
	result.iret = pDBCur->put(pDBCur, pTXN, & DBT1, & DBT2, 0);
	if (0 == result.iret) {
		result.iret = txn_commit(pTXN, DB_TXN_SYNC);
	} else {
		txn_abort(pTXN);
	}
	return &result;
}

/* sync the incore btree's with the disks				(ksb)
 */
void *
loginrsync_1(argp, rqstp)
void *argp;
struct svc_req *rqstp;
{
	static char *pcRet;

	FlushIdx(1);
	sync();
	return (void *)&pcRet;
}

/* ask a class for "info", or a control optoion				(ksb)
 */
info_res *
classrinfo_1(argp, rqstp)
info_param *argp;
struct svc_req *rqstp;
{
	static info_res info_buf;
	register int (*pfiInfo)();

	info_buf.iret = -1;
	if ((char *)0 == argp->pcclass) {
		info_buf.pcdata = "no NULL class, please";
		return & info_buf;
	}
	if ((int (*)())0 == (pfiInfo = class_entry(argp->pcclass, "info"))) {
		info_buf.pcdata = "no info method";
	} else {
		info_buf.iret = pfiInfo(argp->pcopts, & info_buf.pcdata); 
	}
	return & info_buf;

}
%%
