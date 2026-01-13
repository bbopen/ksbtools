#!mkcmd
# Remote Customer Data Service (RPC)					(ksb)
# This is the main program for the remote Customer Data Service
# RPC daemon.
from "<string.h>"
from "<unistd.h>"
from "<sys/types.h>"
from "<limits.h>"
from "<fcntl.h>"
from "<sys/stat.h>"
from "<db.h>"
from "<dirent.h>"
from "<rpc/rpc.h>"
from '<dlfcn.h>'

from '"machine.h"'
from '"main.h"'
from '"uuid.h"'

require "shared.m"
require "util_sigret.m" "util_errno.m" "std_help.m" "util_ppm.m" "std_version.m"
require "xdr_Cstring.m" "xdr_Cargv.m"
require "rcds-inc,xdr,server.x"

%i
static char rcsid[] =
	"$Id: rcds.m,v 2.6 2000/06/06 15:00:42 ksb Exp $";

static int argcSetup;	/* params from the commad line passed to setup	*/
static char **argvSetup;/* for each class we load, no defined meaning	*/
static char *apcFake[2];


#include "callback.hdr"
%%

basename "rcds" ""

type "rpc_server" named "rcdsprog_1" {
	key 1 {
		"RCDSPROG" "RCDSVERS"
	}
}

# options like the stater, only more so -- ksb
char* 'D' {
	named "pcUnitDir"
	init '"/usr/local/lib/rcds"'
	param "dir"
	help "unit dynamic modules location"
}

char* 'R' {
	named "pcRoot"
	init '"/var/rcds"'
	param "root"
	help "the top of the data heirarchy"
}

after {
}

list {
	param "args"
	help "extra arguments to setup units"
	ends
}

%c

/* Data routines (to find a Customer's private data)			(ksb)
 * Each customer is represented here by a UUID found in the top level
 * index (idx).  That attribute, in combination with a primary data
 * server (P), is used here to locate a UNIX directory which holds all
 * of the data units for that Customer.
 *
 * A Customer withe UUID of 1ABC987tuv might have a path (from our
 * $CWD) of:
 *	1A/BC/98/7t/uv
 * Each unit of that directory is represented by a file or directory
 * with the same name as the unit.  The unit "foobie" would be a plain
 * file named "foobie" or a directory "foobie".  Any file with a ".", or
 * and file under a directory can not be a unit.
 *
 * A unit name of the form  "foobie:bletch" is take to make "foobie" with
 * a sub-unit name of "bletch".  The module "foobie" will handle the
 * meaning of a "sub-unit", we just give control to them.
 */

/* put us in the correct place to run					(ksb)
 * open the master index's
 */
void
after_func()
{
	if (0 != chdir(pcRoot)) {
		fprintf(stderr, "%s: chdir: %s: %s\n", progname, pcRoot, strerror(errno));
		exit(1);
	}
}

/* Does this Customer  exist?						(ksb)
 * A Customer exists if the directory exists and has the setgid
 * bit set on it. 0 = yes, 1 = no, -1 = error
 */
int *
uuidrexists_1(argp, rqstp)
exists_param *argp;
struct svc_req *rqstp;
{
	static int iRet;
	register char *pcPath;
	auto struct stat stCust;

	pcPath = MapUUID(argp->pcuuid);
	if ((char *)0 == pcPath) {
		iRet = -1;
		return & iRet;
	}
	if (-1 == stat(pcPath, & stCust)) {
		iRet = 1;
	} else {
		iRet = (0 != (stCust.st_mode & 02000)) ? 0 : 1;
	}

	return &iRet;
}

/* mkdir -p								(ksb)
 */
int
Mkdir_R(char *pcPath)
{
	register char *pcSlash;

	if (0 == mkdir(pcPath, 0777) || EEXIST == errno) {
		return 0;
	}
	if (ENOENT != errno) {
		return -1;
	}

	/* Try to make parent directory when we got back ENOENT, and
	 * there is a "/" in the path we have
	 */
	if ((char *)0 != (pcSlash = strrchr(pcPath, '/'))) {
		register int iGot;

		/* read a//b correctly, and don't leave "" as the target
		 */
		while (pcSlash != pcPath && '/' == pcSlash[-1]) {
			--pcSlash;
		}
		if (pcSlash == pcPath) {
			return -1;
		}

		*pcSlash = '\000';
		iGot = Mkdir_R(pcPath);
		*pcSlash = '/';
		if (0 != iGot) {
			pcSlash = (char *)0;
		}
	}
	return (char *)0 == pcSlash ? -1 : mkdir(pcPath, 0777);
}

/* create the uuid's tree						(ksb)
 * 0 for OK, 1 for fail, 2 for exists, -1 for error
 *
 * When the directory exists just set the setgid to show it is a "home dir"
 * else make it and chmod it.
 */
int *
uuidrnew_1(argp, rqstp)
exists_param *argp;
struct svc_req *rqstp;
{
	static int iRet;
	register char *pcPath;
	auto struct stat stCust;

	pcPath = MapUUID(argp->pcuuid);
	if ((char *)0 == pcPath) {
		iRet = -1;
		return & iRet;
	}
	if (-1 == stat(pcPath, & stCust)) {
		iRet = Mkdir_R(pcPath);
	} else if (0 != (stCust.st_mode & 02000)) {
		iRet = 2;
	}
	if (0 == iRet) {
		iRet = chmod(pcPath, stCust.st_mode | 02751);
	}
	return &iRet;
}

/* index the Custmers units						(ksb)
 */
static PPM_BUF PPMStrs, PPMIndex;
%%
init 3 "util_ppm_init(& PPMStrs, sizeof(char), 1024);"
init 3 "util_ppm_init(& PPMIndex, sizeof(char *), 32);"
%%
Cargv *
uuidrindex_1(argp, rqstp)
exists_param *argp;
struct svc_req *rqstp;
{
	static Cargv ppcRet;
	register char *pcPath, *pcMem;
	register DIR *pDI;
	register struct dirent *pDE;
	register int i, iLen, iNeed;

	ppcRet = (char **)0;
	pcPath = MapUUID(argp->pcuuid);
	if ((char *)0 == pcPath) {
		return & ppcRet;
	}

	/* Open the directory, save the file names and count them
	 */
	if ((DIR *)0 == (pDI = opendir(pcPath))) {
		return & ppcRet;
	}
	iLen = 0;
	for (i = 0; (struct dirent *)0 != (pDE = readdir(pDI)); ) {
		register char *pc;
		register int iCount;
		if ((char *)0 != strchr((pc = pDE->d_name), '.'))
			continue;
		++i;
		iCount = strlen(pc)+1;
		iNeed = iLen+iCount+1;	/* +1 for the nss option */
		pcMem = (char *)util_ppm_size(& PPMStrs, iNeed);
		strcpy(pcMem+iLen, pc);
		iLen += iCount;
	}
	closedir(pDI);

	/* Build the argv return structure, we could just return the
	 * nul-sep-str, but C (other) programmer don't use those much.
	 * (i+1 for the sentinal (char *)0 on the end)  Oh, and I don't
	 * and an xdr encoder/decode for nss's.		--ksb
	 */
	ppcRet = util_ppm_size(& PPMIndex, i+1);
	ppcRet[i] = (char *)0;
	while (i-- > 0) {
		ppcRet[i] = pcMem;
		pcMem += strlen(pcMem)+1;
	}

	return &ppcRet;
}

/* Methods we know about for sure.  If you add one put it here		(ksb)
 * and fix the fallback check below.
 */
char
	acSetup[] = "setup",
	acInit[] = "init",
	acPut[] = "put",
	acGet[] = "get",
	acInfo[] = "info",
	acSearch[] = "search",
	acUncache[] = "uncache",
	acCleanup[] = "cleanup";

/* the method that always returns -2, for "no such method"		(ksb)
 */
static int
meth_fail_int()
{
	return -2;
}


/* Register a unit module for global reference				(ksb)
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
unit_module(pcName, pDLModule)
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


/* record unit calls for fast access					(ksb)
 */
static PPM_BUF PPMUnits;
static int iUnits = 0;
%%
init 3 "util_ppm_init(& PPMUnits, sizeof(UNIT_CALLS *), 16);"
%%
void
unit_register(UNIT_CALLS *pUC)
{
	register UNIT_CALLS **ppUCList;

	if ((UNIT_CALLS *)0 == pUC)
		return;
	ppUCList = (UNIT_CALLS **)util_ppm_size(& PPMUnits, iUnits+1);
	ppUCList[iUnits++] = pUC;
}

/* Return the list of all the info we can, our and the units loaded	(ksb)
 * We ignore the uuid and the unit name, for the present LLL
 */
static PPM_BUF PPMOurInfo;
%%
init 3 "util_ppm_init(& PPMOurInfo, sizeof(char *), 32);"
%%
search_res *
uuidrsearch_1(new_param *pNP)
{
	static search_res SRRet;
	static char acMany[32];
	register char **ppc;
	register int i;
	static char *apcError[] = { "unknown command", (char *)0 };

	SRRet.iret = -1;
	SRRet.ppcfound = apcError;
	if ((char *)0 != pNP->pcopts && 0 == strcmp(pNP->pcopts, "mods")) {
		ppc = util_ppm_size(& PPMOurInfo, iMods+2);
		SRRet.ppcfound = ppc;
		sprintf(acMany, "%d modules", iMods);
		*ppc++ = acMany;
		for (i = 0; i < iMods; ++i) {
			*ppc++ = pMTAll[i].pcname;
		}
		*ppc = (char *)0;
		SRRet.iret = 0;
	}
	if ((char *)0 != pNP->pcopts && 0 == strcmp(pNP->pcopts, "units")) {
		register UNIT_CALLS **ppUCList;

		ppUCList = (UNIT_CALLS **)util_ppm_size(& PPMUnits, 0);
		ppc = util_ppm_size(& PPMOurInfo, iUnits+2);
		SRRet.ppcfound = ppc;
		sprintf(acMany, "%d units", iUnits);
		*ppc++ = acMany;
		for (i = 0; i < iUnits; ++i) {
			*ppc++ = ppUCList[i]->pcunit;
		}
		*ppc = (char *)0;
		SRRet.iret = 0;
	}
	return & SRRet;
}

/* When the unit register'd a fast access UNIT_CALLS we can use		(ksb)
 * that for the well known methods.  This saves lots of calls to
 * dlsym, which were taking time.
 */
void *
unit_cache(UNIT_CALLS *pUC, char *pcMeth)
{
	switch (pcMeth[0]) {
	case 'i':
		if (0 == strcmp(acInit, pcMeth))
			return pUC->pfiinit;
		if (0 == strcmp(acInfo, pcMeth))
			return pUC->pfiinfo;
		break;
	case 'p':
		if (0 == strcmp(acPut, pcMeth))
			return pUC->pfiput;
		break;
	case 'g':
		if (0 == strcmp(acGet, pcMeth))
			return pUC->pfiget;
		break;
	case 's':
		if (0 == strcmp(acSearch, pcMeth))
			return pUC->pfisearch;
		break;
	case 'u':
		if (0 == strcmp(acUncache, pcMeth))
			return pUC->pfiuncache;
		break;
	case 'c':
		if (0 == strcmp(acCleanup, pcMeth))
			return pUC->pficleanup;
		break;
	default:
		break;
	}
	return (void *)0;
}


/* return an entry from the named module, or (void *)0			(ksb)
 * Use the UNIT_CALLS cache if we are a well known entry
 */
void *
unit_entry(pcModule, pcEntry)
char *pcModule, *pcEntry;
{
	register void *pvModule;
	register int (*pfiSetup)();
	register void *pvRet;
	auto char acModPath[MAXPATHLEN+4];
	static UNIT_CALLS *pUCLast;

	if ((void *)0 == (pvModule = unit_module(pcModule, (void *)0))) {
		(void)sprintf(acModPath, "%s/%s.so", pcUnitDir, pcModule);
		pvModule = dlopen(acModPath, RCDS_RTLD_OPTS);
		if ((void *)0 == pvModule) {
			return (void *)0;
		}
		(void)unit_module(pcModule, pvModule);
		pfiSetup = (int (*)())dlsym(pvModule, acSetup);

		/* LLL: Why do we let this fail ?
		 * If they want to exit they should call "ServDown()"
		 * this function might call back to unit_register, if we
		 * are lucky.
		 * When we are force loaded by reference we fake a preload
		 * argv of just the module name.
		 */
		if ((int (*)())0 != pfiSetup) {
			apcFake[0] = pcModule;
			pfiSetup(argcSetup, argvSetup);
		}
	}
	if (0 < iUnits && ((UNIT_CALLS *)0 == pUCLast || 0 != strcmp(pcModule, pUCLast->pcunit))) {
		register UNIT_CALLS **ppUCScan;
		register int i;

		ppUCScan = util_ppm_size(& PPMUnits, 0);
		for (i = 0; i < iUnits; ++i) {
			if (0 != strcmp(ppUCScan[i]->pcunit, pcModule))
				continue;
			pUCLast = ppUCScan[i];
			break;
		}
	}
	if ((UNIT_CALLS *)0 != pUCLast && 0 == strcmp(pcModule, pUCLast->pcunit) && (void *)0 != (pvRet = unit_cache(pUCLast, pcEntry))) {
		return pvRet;
	}

	return (void *)dlsym(pvModule, pcEntry);
}

/* Locate the first subunit spec in the string				(ksb)
 * We take ".", ":" and "/" as unit specs.  For example:
 *	prefs/ship		find the "/"
 *	prefs:track		find the ":"
 *	mad.info		find the ".".
 */
char *
unit_spec(char *pcUnit)
{
	register int c;

	for (/*param*/; '\000' != (c = *pcUnit); ++pcUnit) {
		switch (c) {
		case '/':
		case ':':
		case '.':
			return pcUnit;
		default:
			continue;
		}
	}
	return (char *)0;
}

/* Find the unit method without the trailing spec getting in the way	(ksb)
 * if we can't find it return one that fails with the correc type..
 *
 * N.B. pcUnit must not be a ((const char) *)
 */
void *
unit_meth(char *pcUnit, char *pcMeth)
{
	register char *pcSpec;
	register void *pvRet;

	if ((char *)0 == (pcSpec = unit_spec(pcUnit))) {
		pvRet = unit_entry(pcUnit, pcMeth);
	} else {
		register int iSave;
		iSave = *pcSpec;
		*pcSpec = '\000';
		pvRet = unit_entry(pcUnit, pcMeth);
		*pcSpec = iSave;
	}

	/* Fallback check: we can't take a (void *)0 function back,
	 * so make one up.
	 */
	if ((void *)0 != pvRet) {
		return pvRet;
	}
	return (void *)meth_fail_int;
}

/* load the unit, pass it the path and the other info, it'll build	(ksb)
 * what ever it wants.
 */
int *
unitrnew_1(argp, rqstp)
new_param *argp;
struct svc_req *rqstp;
{
	static int iRet;
	register char *pcPath, *pcChop;
	register int (*pfiInit)();

	pcPath = MapUUID(argp->pcuuid);
	if ((char *)0 == pcPath) {
		iRet = -1;
		return & iRet;
	}
	pcChop = pcPath+strlen(pcPath);
	pfiInit = unit_meth(argp->pcunit, acInit);
	iRet = pfiInit(pcPath, argp->pcunit, argp->pcopts);
	*pcChop = '\000';

	return &iRet;
}

/* put something in the unit						(ksb)
 */
put_res *
unitrput_1(argp, rqstp)
put_param *argp;
struct svc_req *rqstp;
{
	static put_res put_buf;
	register char *pcPath, *pcChop;
	register int (*pfiPut)();

	put_buf.iret = -1;
	put_buf.pcold = (char *)0;
	if ((char *)0 == (pcPath = MapUUID(argp->pcuuid))) {
		return & put_buf;
	}
	pcChop = pcPath+strlen(pcPath);
	pfiPut = unit_meth(argp->pcunit, acPut);
	put_buf.iret = pfiPut(pcPath, argp->pcunit, argp->pcname, argp->pcvalue, & put_buf.pcold);
	*pcChop = '\000';

	return &put_buf;
}

/* get a NVP from the unit						(ksb)
 */
put_res *
unitrget_1(argp, rqstp)
new_param *argp;
struct svc_req *rqstp;
{
	static put_res get_buf;
	register char *pcPath, *pcChop;
	register int (*pfiGet)();

	pcPath = MapUUID(argp->pcuuid);
	if ((char *)0 == pcPath) {
		get_buf.iret = ENOENT;
		get_buf.pcold = (char *)0;
		return &get_buf;
	}
	pcChop = pcPath+strlen(pcPath);
	pfiGet = unit_meth(argp->pcunit, acGet);
	get_buf.iret = pfiGet(pcPath, argp->pcunit, argp->pcopts, &get_buf.pcold);
	*pcChop = '\000';

	return &get_buf;
}

/* search the unit, whatever that means to you				(ksb)
 */
search_res *
unitrsearch_1(argp, rqstp)
new_param *argp;
struct svc_req *rqstp;
{
	static search_res search_buf;
	register char *pcPath, *pcChop;
	register int (*pfiSearch)();

	pcPath = MapUUID(argp->pcuuid);
	if ((char *)0 == pcPath) {
		search_buf.iret = ENOENT;
		search_buf.ppcfound = (char **)0;
		return &search_buf;
	}
	pcChop = pcPath+strlen(pcPath);
	pfiSearch = unit_meth(argp->pcunit, acSearch);
	search_buf.iret = pfiSearch(pcPath, argp->pcunit, argp->pcopts, &search_buf.ppcfound);
	*pcChop = '\000';

	return &search_buf;
}

/* call the info method for the unit					(ksb)
 */
put_res *
unitrinfo_1(argp, rqstp)
new_param *argp;
struct svc_req *rqstp;
{
	static put_res info_buf;
	register char *pcPath, *pcChop;
	register int (*pfiInfo)();

	pcPath = MapUUID(argp->pcuuid);
	if ((char *)0 == pcPath) {
		info_buf.iret = ENOENT;
		info_buf.pcold = (char *)0;
		return & info_buf;
	}
	pcChop = pcPath+strlen(pcPath);
	pfiInfo = unit_meth(argp->pcunit, acInfo);
	info_buf.iret = pfiInfo(pcPath, argp->pcunit, argp->pcopts, & info_buf.pcold);
	*pcChop = '\000';

	return &info_buf;
}

/* sync the cache for this UUID, only called when testing (mostly)	(ksb)
 * Really syncs the cache for _every_ uuid, because that is easy.
 */
int *
unitrsync_1(argp, rqstp)
new_param *argp;
struct svc_req *rqstp;
{
	static int iRet;
	register char *pcUnit;
	register int (*pfiSync)();

	if ((char *)0 != (pcUnit = argp->pcunit)) {
		pfiSync = unit_meth(pcUnit, acUncache);
		iRet = pfiSync(argp);
	} else {
		sync();
		iRet = 0;
	}
	return & iRet;
}

/* Shutdown the data services and give the units a chance to sync	(ksb)
 * or better yet cleanup all together.
 */
SIGRET_T
ServDown()
{
	register int i, iRet;
	register int (*pfiCleanup)();

	iRet = 0;
	for (i = 0; i < iMods; ++i) {
		pfiCleanup = unit_meth(pMTAll[i].pcname, acCleanup);
		if ((int (*)())0 == pfiCleanup)
			pfiCleanup = unit_meth(pMTAll[i].pcname, acUncache);
		if ((int (*)())0 == pfiCleanup)
			continue;
		iRet |= pfiCleanup((new_param *)0);
	}
	exit(iRet);
}


/* load the class before we even take the first request			(ksb)
 */
static void
PreLoad(ppcArgs)
char **ppcArgs;
{
	register char *pcUnit;
	register int i;

	if ((char **)0 == ppcArgs || (char *)0 == (pcUnit = *ppcArgs)) {
		return;
	}
	i = 0;
	argvSetup = ppcArgs;
	while ((char *)0 != *ppcArgs) {
		++ppcArgs, ++i;
	}
	argcSetup = i;
	printf("%s: preload unit %s(%d)\n", progname, pcUnit, i);
	if ((void *)0 == unit_entry(pcUnit, acSetup)) {
		fprintf(stderr, "%s: %s: %s entry missing -- preload failed\n", progname, pcUnit, acSetup);
		exit(10);
	}
}

/* run any pre-loads just like rcls does				(ksb)
 */
int
list_func(argc, argv)
int argc;
char **argv;
{
	register char **ppcDash, *pcParam;
	extern SIGRET_T ServDown();

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
	signal(SIGTERM, ServDown);
	u_rpc_reg(RPC_ANYSOCK);
	svc_run();
	exit(0);
}
%%
