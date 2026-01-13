#!mkcmd
# $Id: sso.m,v 2.5 2000/07/12 00:26:50 ksb Exp $
# the example class interface file					(ksb)
#
# In the same model as WISE's stater we are dynamically loaded to
# install a Customer in the remote customer database.  The master
# index picks a "class" to:
#	+ Find a UUID and other protected fields
#	+ Set any manditory public fields
#	+ Build minimal linkage to get to the next level (rcds)
#
from '<sys/types.h>'
from '<sys/param.h>'
from '<sys/time.h>'
from '<sys/stat.h>'
from '<strings.h>'
from '<fcntl.h>'
from '<stdio.h>'
from '<stdlib.h>'
from '<limits.h>'
from '<sys/socket.h>'
from '<netinet/in.h>'
from '<netdb.h>'

from '"rcls_callback.h"'
from '"machine.h"'
from '"sso.h"'

require "xdr_Cstring.m" "xdr_Cargv.m"
require "std_help.m" "std_version.m" "util_ppm.m"
require "expladdr.m"
require "hash-pipe.m"
require "../rcds/rcds-xdr,header,client.x"
require "sso-post.m"


%i
/* our version information
 */
static char rcsid[] =
	"$Id: sso.m,v 2.5 2000/07/12 00:26:50 ksb Exp $";

static PPM_BUF PPMInfo;
%%

init 3 "util_ppm_init(& PPMInfo, sizeof(char ), 1024);"
init "dump_hdr(argc, argv);"

%c
static int
dump_hdr(int argc, char **argv)
{
#if DEBUG
	register char *pcSep;
#endif

#if DEBUG
	fprintf(stderr, "%s\n", rcsid);
	pcSep = "";
	while ((char *)0 != *argv) {
		fprintf(stderr, "%s%s", pcSep, *argv);
		++argv, pcSep = " ";
	}
	fprintf(stderr, "\n");
#endif
	return 0;
}

typedef struct RHnode {
	CLIENT *pRC;		/* RPC client to chat with host		*/
	char *pcserv;		/* server name in DNS, not an IP!	*/
	int iincl;		/* is the name in the last RR we got	*/
	int ifail;		/* didn't open when we tried last	*/
} RR_HOST;
static PPM_BUF PPMHosts;
static int iHold = 0;

%%
init 3 "util_ppm_init(& PPMHosts, sizeof(RR_HOST), 32);"

integer 't' {
	named "iRRCache"
	init "5 * 60"
	param "timeout"
	help "keep the round-robin at most timeout seconds"
}
char* 'R' {
	named "pcRR"
	init '"rcds"'
	param "name"
	help "rcds round-robin DNS record to build new logins"
}
%c
/* if the server is in the list take it out and shorten the list	(ksb)
 */
static char *
RRMember(char *pcServ, char **ppcList, int *piCount)
{
	register int i;
	register char *pcRet;

	pcRet = (char *)0;
	for (i = *piCount; i-- > 0; ++ppcList) {
		if (0 != strcmp(*ppcList, pcServ)) {
			continue;
		}
		pcRet = ppcList[0];
		ppcList[0] = ppcList[i];
		ppcList[i] = (char *)0;
		--(*piCount);
		break;
	}
	return pcRet;
}

/* We think it is time to update the RR cache				(ksb)
 *	get the list of new hosts from DNS (with ExplAddr)
 *	resize to add new ones (or if none return)
 *	mark the last group as not included (in case none are)
 *	all the common ones mark as incl and free,
 *	add the unloved new ones to end of list and return new size
 *
 * N.B. to handle the case when forward and back change for a host we
 *	have to close all the open (CLIENT *)'s.  Sad but the best way.
 */
static int
UpdateRR(RR_HOST **ppRRList)
{
	register RR_HOST *pRRStat;
	register int i;
	auto int iNew;
	register char **ppcList, **ppcScan, *pcFound;
	register struct hostent *pHERR;

	if ((struct hostent *)0 == (pHERR = gethostbyname(pcRR))) {
		fprintf(stderr, "%s: %s: %s: host lookup failure\n", progname, sso_progname, pcRR);
		return iHold;
	}
	pHERR = util_savehostent(pHERR, pcRR);
	if ((struct hostent *)0 == pHERR) {
		fprintf(stderr, "%s: %s: no core\n", progname, sso_progname);
		return iHold;
	}
	ppcList = ExplAddr(pHERR);
	free((void *)pHERR);
	iNew = 0;
	for (ppcScan = ppcList; (char *)0 != *ppcScan; ++ppcScan) {
		++iNew;
	}

	pRRStat = (RR_HOST *)util_ppm_size(& PPMHosts, iHold+iNew);
	if ((RR_HOST *)0 == pRRStat) {
		fprintf(stderr, "%s: %s: %d: round-robin too big\n", progname, sso_progname, iHold+iNew);
		return -1;
	}
	for (i = 0; i < iHold; ++i) {
		if ((CLIENT *)0 != pRRStat[i].pRC) {
			clnt_destroy(pRRStat[i].pRC);
			pRRStat[i].pRC = (CLIENT *)0;
		}
		if ((char *)0 == (pcFound = RRMember(pRRStat[i].pcserv, ppcList, & iNew))) {
			pRRStat[i].iincl = 0;
			continue;
		}
		free((void *)pcFound);
		pRRStat[i].iincl = 1;
		pRRStat[i].ifail = 0;
	}
	/* bzero((void *)& pRRStat[iHold], iNew * sizeof(RR_HOST)); */
	for (ppcScan = ppcList; (char *)0 != *ppcScan; ++ppcScan) {
		pRRStat[i].pcserv = *ppcScan;
		pRRStat[i].pRC = (CLIENT *)0;
		pRRStat[i].iincl = 1;
		pRRStat[i].ifail = 0;
		++i;
	}
	*ppRRList = pRRStat;
	return iHold = i;
}

/* get the next active host in our RCDS RR record, or so		(ksb)
 *
 * We re-read the RR record (-R) every 5 minutes (-t), we present the
 * hosts we read last time in round-robin until the next timeout, we
 * only try to connect to each rcds once every 5 minutes (-t) if it
 * fails to connect.
 *
 * INV: we never get fewer hosts in the RRlist, they just get
 * marked with iincl = 0
 */
static RR_HOST *
RRHost(int iProg, int iVer, char *pcTrans)
{
	static time_t tLast;
	static RR_HOST *pRRList = (RR_HOST *)0;
	static int iHave, iCur = -1;
	register int i;
	register RR_HOST *pRRTry;
	auto time_t tNow;
	auto struct timeval tvOut;

	time(& tNow);
	if (tLast + iRRCache < tNow || (RR_HOST *)0 == pRRList) {
		iHave = UpdateRR(& pRRList);
		tLast = tNow;
	}
	if ((RR_HOST *)0 == pRRList || iHave < 1) {
		return (RR_HOST *)0;
	}
	/* look for a member that
	 * is included, doesn't fail, and has a non-NULL CLIENT *...
	 */
	for (i = 0; i < iHave; ++i) {
		++iCur;
		if (iCur >= iHave) {
			iCur = 0;
		}
		pRRTry = & pRRList[iCur];
		if (0 != pRRTry->ifail || 0 == pRRTry->iincl) {
			continue;
		}
		if ((CLIENT *)0 != pRRTry->pRC) {
			return pRRTry;
		}
		if ((CLIENT *)0 == (pRRTry->pRC = clnt_create(pRRTry->pcserv, iProg, iVer, pcTrans))) {
			clnt_pcreateerror(pRRTry->pcserv);
			pRRTry->ifail = 1;
			continue;
		}
		/* calls timeout after ~3 seconds -- ksb */
		tvOut.tv_sec = 3;
		tvOut.tv_usec = 0;
		(void)clnt_control(pRRTry->pRC, CLSET_TIMEOUT, (char *)& tvOut);
		return pRRTry;
	}
	return (RR_HOST *)0;
}

/* return the next open rcds host that honors our create request	(ksb)
 * Get a RR record and cache it for a while.
 */
static char *
NextHost(char *pcUuid, char *pcOut)
{
	register int *piRet;
	register RR_HOST *pRR;
	auto exists_param EPNew;

	if ((char *)0 == pcOut) {
		return pcOut;
	}

	/* Call the RPC to setup the rcds cache for the Customer.
	 */
	while ((RR_HOST *)0 != (pRR = RRHost(RCDSPROG, RCDSVERS, "tcp"))) {
		EPNew.pcuuid = pcUuid;
		piRet = uuidrnew_1(&EPNew, pRR->pRC);

		if ((int *)0 == piRet) {
			pRR->ifail = 1;
			continue;
		}
		/* If we get back a "read-only" return we should try the next,
		 * a -1 means we can't ever win because (I guess) the UUID
		 * is malformed, so don't keep trying them over and over.
		 */
		if (-1 == *piRet) {
			return (char *)0;
		}
		if (0 == *piRet) {
			return strcpy(pcOut, pRR->pcserv);
		}
		/* LLL check the return code for special cases?
		 */
	}
	return (char *)0;
}

/* create the login and initalize attributes we must have		(ksb)
 * return 0 for add, others for failure.
 *
 * Each SSO class Customer gets these protected arrtibutes:
 *	a primary database engine (P)
 *	a Unique User IDentifier (UUID)
 */
int
init(argp, pppcProtect, pppcPublic)
create_param *argp;
char ***pppcProtect, ***pppcPublic;
{
	static mattr_buf MAPro;
	static char acHash[8+(HASH_LENGTH|3)+1] =	/* UUID=<HASH>\0 */
#define HASH_START	2
		"U=";
	static char acPHost[8+MAXHOSTNAMELEN+4] =	/* P=<rcds-host>\0 */
#define HOST_START	2
		"P=";
	static char *apcDummy[3] = {
		acHash,
		acPHost,
		(char *)0
	};


	if ((char *)0 == NewHash(acHash+HASH_START)) {
		return 430;
	}
	if ((char *)0 == NextHost(acHash+HASH_START, acPHost+HOST_START)) {
		return 431;
	}
	*pppcProtect = argp->ppcprotect;
	(void)MergeAttrs(&MAPro, pppcProtect, apcDummy);
	*pppcPublic = argp->ppcpublic;

	/* We do _not_ build any other rcds things: we let the stater do it.
	 */
	return 0;
}

/* info/status request							(ksb)
 * Also used as a control point al-la ioctl(2).
 * We can return the present RR contents via "rr".
 */
int
info(pcCntl, ppcOut)
char *pcCntl, **ppcOut;
{
	register int i, iCnt, iRet;
	register char *pcMem, *pcSep;
	static char acInfo[] = "round-robin hosts: ",
		acComma[] = ", ",
		acNone[] = "none";

	iRet = 0;
	if ((char **)0 != ppcOut) {
		*ppcOut = "command not recognized";
	}
	if ((char *)0 != pcCntl && 0 == strcmp(pcCntl, "rr")) {
		register RR_HOST *pRRInfo;

		pRRInfo = (RR_HOST *)util_ppm_size(& PPMHosts, 0);
		iCnt = sizeof(acInfo) > sizeof(acNone) ? sizeof(acInfo) : sizeof(acNone);
		for (i = 0; i < iHold; ++i) {
			iCnt += strlen(pRRInfo[i].pcserv)+sizeof(acComma);
		}
		pcMem = util_ppm_size(& PPMInfo, iCnt+1);
		if ((char **)0 != ppcOut) {
			*ppcOut = pcMem;
		}
		pcSep = acInfo;
		for (i = 0; i < iHold; ++i) {
			(void)strcpy(pcMem, pcSep);
			(void)strcat(pcMem, pRRInfo[i].pcserv);
			pcSep = acComma;
			pcMem += strlen(pcMem);
		}
		if (0 == iHold) {
			(void)strcpy(pcMem, acNone);
		}
	}
	return iRet;
}

/* setup any dynamic strucutre we need to do our work, called once	(ksb)
 * when the RPC daemon loads us, same as a unit in the data layer.
 * Must have one to be a class module.
 */
/* mkcmd build's setup */


/* Cleanup any in-core strucutre we built that we must on the way down:	(ksb)
 * here we close any RPC CLIENT handles that we might have open.
 */
int
cleanup()
{
	register int i;
	register RR_HOST *pRRClose;

	pRRClose = (RR_HOST *)util_ppm_size(& PPMHosts, 0);
	if ((RR_HOST *)0 == pRRClose) {
		return 0;
	}
	for (i = 0; i < iHold; ++i) {
		if ((CLIENT *)0 == pRRClose[i].pRC) {
			continue;
		}
		clnt_destroy(pRRClose[i].pRC);
		pRRClose[i].pRC = (CLIENT *)0;
	}
	return 0;
}
%%
