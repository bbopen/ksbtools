/*@Version $Revision: 2.6 $@*/
/*@Header@*/
/* $Id: wise.c,v 2.6 2009/10/16 15:41:17 ksb Exp $
 * state-side WISE support code
 * Plus Modules to read/write a RFC 821 stype reply from file
 * descriptors	(ksb)
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>

/*@Shell %echo ${MACHINE_H+"$MACHINE_H"}%@*/
#include "wise.h"

/*@Explode pad@*/
long int wise_pad;

/* set a one-time-pad 32 bit hasher, called in the state sequence	(ksb)
 * just before we fork() a new state
 */
void
wise_setpad(iRand)
long int iRand;
{
	wise_pad = iRand;
}


/*@Explode timer@*/
unsigned int wise_timeout = WISE_TIMEOUT;

/* set the interval timer for session timeout				(ksb)
 */
unsigned int
wise_settimeout(iTimer)
unsigned int iTimer;
{
	register unsigned int iOld;

	iOld = wise_timeout;
	wise_timeout = iTimer;
	alarm(iTimer);
	return iOld;
}

/*@Explode port@*/
/* bind to a high TCP/IP port to service our Customer			(ksb)
 */
int
wise_port(piPort)
int *piPort;
{
	register int fd;
	auto struct sockaddr_in wise_portspec;

	wise_portspec.sin_family = AF_INET;
	wise_portspec.sin_addr.s_addr = htonl(INADDR_ANY);;
	wise_portspec.sin_port = htons((u_short)0);
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return -1;
	}
	if (bind(fd, (struct sockaddr *)&wise_portspec, sizeof(wise_portspec)) < 0) {
		return -1;
	}
	(void)listen(fd, 20);
	if ((int *)0 != piPort) {
		auto int iSpec;
		iSpec = sizeof(wise_portspec);
		if (-1 != getsockname(fd, (struct sockaddr *)&wise_portspec, & iSpec)) {
			*piPort = ntohs(wise_portspec.sin_port);
		} else {
			*piPort = 0;
		}
	}
	return fd;
}

/*@Explode name@*/
/* Pick a key to remember this host, we get the hostname,		(ksb)
 * chop at the left most ., and remove "sso" if it is on the front.
 * That picks it.  In the attach code if the key starts with
 * a digit we prefix "sso" and postfix "".  If the code starts
 * with a letter we postfix ".zmd.fedex.com".
 */
char *
wise_name(pcKey)
char *pcKey;
{
	auto char acMyname[256];
	register char *pcChop;

	if (-1 == gethostname(acMyname, sizeof(acMyname))) {
		return "localhost";
	}
	if ((char *)0 != (pcChop = strchr(acMyname, '.'))) {
		*pcChop = '\000';
	}
	if ('s' == acMyname[0] && 's' == acMyname[1] && 'o' == acMyname[2] && isdigit((int)acMyname[3])) {
		return strcpy(pcKey, acMyname+3);
	}
	return strcpy(pcKey, acMyname);
}

/*@Explode host@*/
/* undo what the encode above did for us				(ksb)
 * uncompact the name of the host that holds our state
 */
char *
wise_host(pcKey, pcHost)
char *pcKey, *pcHost/* [256] */;
{
	if (isdigit((int)*pcKey))
		sprintf(pcHost, "sso%s", pcKey);
	else
		sprintf(pcHost, "%s", pcKey);
	return pcHost;
}

/*@Explode module@*/
/* Register a wise module for global reference				(ksb)
 * Give me an entry and I'll record it, give be (void *)0 and I'll find
 * it.  When you are recording you should strdup any dynamic names.
 *
 * We are not very clever here -- just linear scans. -- ksb
 */
MOD_ENTRY *pMTAll = (MOD_ENTRY *)0;
int iMods = 0;
void *
wise_module(pcName, pDLModule)
char *pcName;
void *pDLModule;
{
	register int i;
	static int iMaxMods = 0;

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

/* Load the module from the path in the filesystem, default to		(ksb)
 * the application home on a (char *)0 path.  Return -1 for not loaded,
 * 0 for still loaded, 1 for newly loaded.
 * It is kinda a bug that the module "fred" can only be found once,
 * even if you provide a different search path the second time {XXX}.
 */
int
wise_load(pcModule, pcPath)
char *pcModule, *pcPath;
{
	register void *pvDone;
	register char *pcModPath, *pcCursor, *pcColon;

	pvDone = wise_module(pcModule, (void *)0);
	if ((void *)0 != pvDone) {
		return 0;
	}

	if ((char *)0 == pcPath) {
		pcPath = WISE_MOD_PATH;
	}
	/* /opt/sso/lib/wise_apply "/" argv[0] ".so" '\000'
	 * when the path contains a colon we are just way too big.
	 */
	pcModPath = calloc(((strlen(pcPath)+1+strlen(pcModule)+1)|7)+1, 1);
	if ((char *)0 == pcModPath) {
		errno = ENOMEM;
		return -1;
	}
	for (pcCursor = pcPath; (void *)0 == pvDone && (char *)0 != pcCursor && '\000' != *pcCursor; pcCursor = pcColon) {
		if ((char *)0 == (pcColon = strchr(pcCursor, ':')))
			pcColon = strchr(pcCursor, '\000');
		(void)sprintf(pcModPath, "%.*s/%s.so", pcColon-pcCursor, pcCursor, pcModule);
		if (':' == *pcColon)
			++pcColon;
		pvDone = dlopen(pcModPath, WISE_RTLD_OPTS);
	}
	if ((void *)0 == pvDone) {
		return -1;
	}
	(void)wise_module(pcModule, pvDone);
#if NEED_FREE_ALL
	free((void *)pcModPath);
#endif
	return 1;
}

/* Return an entry from the named module, or (void *)0;			(ksb)
 * the module _must_ be loaded already.
 */
wise_entry_t
wise_entry(pcModule, pcEntry)
char *pcModule, *pcEntry;
{
	register void *pvModule;

	pvModule = wise_module(pcModule, (void *)0);
	if ((void *)0 == pvModule) {
		return (wise_entry_t)0;
	}
	return (wise_entry_t)dlsym(pvModule, pcEntry);
}

/*@Explode status@*/
/* Update the global status file for the slot we are in, the special	(ksb)
 * slot WISE_OUR_SLOT (-2) means the same slot we updated last time.
 * The slot WISE_HEADER_SLOT (-1) is the slot for the stater.
 * Each slot can have up to 79 characters of data (padded with spaces on
 * the end) + a '\n' for 80 total.
 *
 * The mystic WISE_RESET_SLOT (-3) means truncate the file to start over
 * in that case we return the value from ftrucate, else the slot we updated.
 * In all cases -1 might be bad (but expected for WISE_HEADER_SLOT calls).
 */
int
wise_status(iSlot, pcTxt)
int iSlot;
char *pcTxt;
{
	static char acStatDB[] = WISE_STATDB;
	static int iLast = -1;
	register int fdStatus;
	register off_t lWhere;
	register char *pcStatDB;
	auto char acTag[WISE_DB_LINE+2];

	if (WISE_OUR_SLOT == iSlot) {
		iSlot = iLast;
	}
	if ((char *)0 != (pcStatDB = getenv("WISE_DB"))) {
		if (-1 == (fdStatus = open(pcStatDB, O_RDWR|O_NOCTTY, 0666))) {
			return 0;
		}
	} else if (-1 == (fdStatus = open(acStatDB, O_RDWR|O_NOCTTY, 0666))) {
		return 0;
	}
	if (WISE_RESET_SLOT == iSlot) {
		register int iRet;

		iRet = ftruncate(fdStatus, (off_t)0);
		close(fdStatus);
		return iRet;
	}

	/* must be a real update
	 */
	if (iSlot < -1) {
		iSlot = -1;
	}
	lWhere = (WISE_DB_LINE+1L)*(1L+iSlot);
	if ((char *)0 == pcTxt) {
		pcTxt = "";
	}
	sprintf(acTag, "%*.*s\n", WISE_DB_LINE, WISE_DB_LINE, pcTxt);
	if (-1 != lseek(fdStatus, lWhere, SEEK_SET)) {
		(void)write(fdStatus, acTag, WISE_DB_LINE+1);
	}
	close(fdStatus);
	return iLast = iSlot;
}

/*@Explode put@*/
/* Output the text in a RFC821 complient format				(ksb)
 * We could put a wrapper around this to use the pcText to lookup
 * and International text string, but the number is the key here.
 * Just don't put a naked \r in the text, ok?
 * If no text is given we pad with "one of 6 defaults"
 */
int
wise_put(fd, iNum, pcText)
int fd, iNum;
const char *pcText;		/* C style message, might be missing last \n */
{
	register const char *pcNl;
	register char *pcNext;
	register int iNl, iPad, cc;
	auto char acDigits[32];
	static int iNeed = 0, iHave = 0;
	static char *pcBuffer = (char *)0;

	if ((char *)0 == pcText || '\000' == *pcText) {
		static char *apcDefs[] = {
			"unknown", "ok", "complete", "working", "sorry", "no"
		};
		cc = iNum / 100;
		if (cc < 0 || cc >= sizeof(apcDefs)/sizeof(apcDefs[0])) {
			cc = 0;
		}
		pcText = apcDefs[cc];
	}
	iNl = 0, pcNl = pcText;
	while ((char *)0 != (pcNext = strchr(pcNl, '\n'))) {
		++iNl;
		pcNl = pcNext + 1;
	}
	if ('\000' != *pcNl) {
		++iNl;
	}
	/* each line gets:
	 *	<digits>[- ]$text_for_this_line\r\n
	 *	+ \000 on the end
	 * which is exactly:
	 */
	if (-1 == iNum) {
		acDigits[0] = '\000';
	} else {
		(void)sprintf(acDigits, "%03d", iNum);
	}
	iPad = iNl*(strlen(acDigits)+1+2)+1;
	iNeed = iPad + strlen(pcText);

	if (iHave < iNeed) {
		iNeed |= 15;
		++iNeed;
		if ((char *)0 == pcBuffer) {
			pcBuffer = malloc(iNeed);
		} else {
			pcBuffer = realloc((void *)pcBuffer, iNeed);
		}
	}
	if ((char *)0 == (pcNext = pcBuffer)) {
		errno = ENOMEM;
		return -1;
	}

	while (0 != iNl--) {
		(void)strcpy(pcNext, acDigits);
		pcNext += strlen(pcNext);
		*pcNext++ = 0 != iNl ? '-' : ' ';
		while ('\n' != *pcText && '\000' != *pcText) {
			if ('\r' == *pcText) {
				++pcText;
				continue;
			}
			*pcNext++ = *pcText++;
		}
		*pcNext++ = '\r';
		*pcNext++ = '\n';
		if ('\n' == *pcText) {
			++pcText;
		}
	}
	*pcNext = '\000';
	iPad = pcNext - pcBuffer;
	pcNl = pcBuffer;
	while (iPad > 0 && -1 != (cc = write(fd, pcNl, iPad))) {
		iPad -= cc;
		pcNl += cc;
	}
	return iPad;
}

/*@Explode get@*/
/* Read an arbitrary RFC821 style reply code				(ksb)
 * The caller should strdup anything they want to keep from  the returned
 * buffer.  We are careful not to read more data than the last character
 * on our line.  We expect CRLF (\r\n) end of lines and are unhappy (block)
 * if we just see <CR> (\r, AKA \015) but will accept just LF (\n, AKA \012).
 *
 * A clean end of file is returned as (-1, (char *)0)
 * A lone "[\r]\n" is returned as (-1, "").
 *
 * As a special case when a line with no code is read, the code is set to -1.
 * If a line with a different code is read (misformatted) then we, well,
 * remember the code for next time.  What a hack.
 *	100-this line should be continued\n
 *	101 this should be tagged 100\n
 * this function will return 100, "this line should be continued" and
 * 101, "this should be tagged 100".  Lines with no code digits are valid
 * "-" lines (be liberal with what you accept).  Stray '\r' are removed.
 * Humm:
 *	100-this line should be continued\n
 *	\n
 * should return (100, "this...\n"), I think.
 *
 * A last special case, EOF is interpreted as you'd expect (as \n if
 * we have read a code).
 */
int
wise_get(fd, piNum, ppcText)
int fd, *piNum;
char **ppcText;
{
#define WS_LEAD	'l'		/* looking for a CODE		*/
#define WS_CODE	'c'		/* have a code, next line	*/
#define WS_TEXT 't'		/* saw text, continue on next	*/
#define WS_LAST 'x'		/* this is the last line	*/
	static char *pcBuffer;
	static int iHave = 0;
	static int iState = WS_LEAD;
	static int iCheck = -1;
	register int iCode, iLen, cc;
	auto char acIn[8];

	if (-1 == fd) {		/* reset our state */
		iState = WS_LEAD;
		iCheck = -1;
		return -1;
	}

	if ((char *)0 == pcBuffer) {
		iHave = 1024;
		pcBuffer = malloc(iHave);
	}
	if ((char *)0 == pcBuffer) {
		errno = ENOMEM;
		return -1;
	}
	iLen = 0;
	if (iState == WS_LEAD) {
		iCode = 0;
	} else {
		iCode = iCheck;
	}
	pcBuffer[0] = '\000';
	while (0 < (cc = read(fd, acIn, 1))) {
		register int c;
		register char *pcReBuf;

		c = acIn[0];
		if ('\000' == c || '\r' == c) {
			continue;
		}
		if (iLen == iHave) {
			iHave += 2048;
			pcReBuf = realloc((void *)pcBuffer, iHave);
			if ((char *)0 == pcBuffer) {
				errno = ENOMEM;
				return -1;
			}
			pcBuffer = pcReBuf;
		}
		if (WS_LEAD == iState || WS_CODE == iState) {
			if (isdigit(c)) {
				if (-1 == iCode) {
					iCode = 0;
				}
				iCode *= 10;
				iCode += c - '0';
				continue;
			}
			if (WS_CODE == iState && iCheck != iCode) {
				register int iTemp;
				iTemp = iCode;
				iCode = iCheck;
				iCheck = iTemp;
				pcBuffer[iLen] = '\000';
				iState = '-' == c ? WS_TEXT : WS_LAST;
				break;
			}
			/* no code, or 000 code
			 */
			if (-1 == iCode) {
				/* fall though */;
			} else if ('-' == c) {
				iState = WS_TEXT;
				continue;
			} else if (' ' == c || '\t' == c) {
				iState = WS_LAST;
				continue;
			}
			/* 431first letter flush with code?, or no code */
			iState = WS_LAST;
		}
		/* end of this line, continue or end string and break
		 */
		if ('\n' == c) {
			if (WS_LAST == iState) {
				iCheck = -1;
				pcBuffer[iLen] = '\000';
				iState = WS_LEAD;
				break;
			}
			if (WS_TEXT == iState) {
				iCheck = iCode;
				iCode = 0;
				iState = WS_CODE;
			}
		}
		pcBuffer[iLen++] = c;
	}
	/* we used to check iCode == 0 here, why?
	 */
	*piNum = 0 == iLen ? -1 : iCode;
	*ppcText = (0 == cc && 0 == iLen) ? (char *)0 : pcBuffer;
	return 0;
}

/* just like wise_get, but skim past 100 series codes			(ksb)
 * These are just padding for some operations, the "c" is for "complete"
 */
int
wise_cget(fd, piNum, ppcText)
int fd, *piNum;
char **ppcText;
{
	register int iRet;
	auto int iNever;

	if ((int *)0 == piNum) {
		piNum = & iNever;
	}
	while (0 == (iRet = wise_get(fd, piNum, ppcText))) {
		if (100 <= *piNum && *piNum < 200)
			continue;
		break;
	}
	return iRet;
}

/*@Remove@*/
#if defined(TEST)
/*@Explode test@*/
/* a test driver for get and put at least				(ksb)
 */
int
main(argc, argv, envp)
int argc;
char **argv, **envp;
{
	auto int iCode;
	auto char *pcMes;

	wise_put(1, 100, "This is simple text, one line\n");
	wise_put(1, 101, "This is simple text, missing LF");
	wise_put(1, 200, "Multi-line test\nhas two lines\n");
	wise_put(1, 201, "Multi-line test\nhas two lines, but one LF");
	wise_put(1, 0, "sprintf width check\n");
	wise_put(1, 10, "");
	wise_put(1, 100, (char *)0);
	wise_put(1, 200, (char *)0);
	wise_put(1, 300, (char *)0);
	wise_put(1, 400, (char *)0);
	wise_put(1, 500, (char *)0);
	wise_put(1, 600, (char *)0);
	wise_put(1, 220, "read test enter reply codes...");
	/* input test */
	while (0 == wise_get(0, & iCode, & pcMes)) {
		if (-1 == iCode && (char *)0 == pcMes) {
			break;
		}
		printf("code=%d, text=\"%s\"\n", iCode, pcMes);
	}
	exit(0);
}
/*@Remove@*/
#endif
