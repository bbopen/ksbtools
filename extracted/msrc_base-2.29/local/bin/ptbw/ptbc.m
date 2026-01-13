#!mkcmd
#@Version $Revision: 1.38 $@#
#@Header@#
# $Id: ptbc.m,v 1.38 2010/02/22 15:04:36 ksb Exp $
from '<stdlib.h>'
from '<unistd.h>'
from '<fcntl.h>'
from '<sys/types.h>'
from '<sys/wait.h>'
from '<sys/socket.h>'
from '<sys/un.h>'
from '<signal.h>'
from '<netdb.h>'
from '<sys/stat.h>'
from '<errno.h>'
from '<string.h>'
from '<stdio.h>'

from '"machine.h"'

%i
static char PTBacVersion[] =
	"0.4\n";
%%
#@Explode pidrecv@#
%hi
extern int PTBPidRecv(int, char *, size_t);
%%
%c
/* given a socket, and that the peer is calling "PTBPidSend" at this	(ksb)
 * point in the protocol, we return the pid of the peer.  Gross on
 * some versions of UNIX (for sure).  We can recv pid@host for wrapw,
 * but we toss the @host part still.
 */
int
PTBPidRecv(int s, char *pcRemember, size_t wMax)
{
	register int i, fNeg;
	register char *pcFrom;
	auto char acPid[1024];

	pcFrom = (char *)0;
	fNeg = 0;
	for (i = 0; i < sizeof(acPid); ++i) {
		if (1 != read(s, acPid+i, 1))
			return -1;
		if ('-' == acPid[i])
			fNeg = 1, --i;
		if (isspace(acPid[i]))
			break;
		if ('@' != acPid[i])
			continue;
		pcFrom = & acPid[i+1];
		acPid[i] = '\000';
	}
	acPid[i] = '\000';
	if ((char *)0 != pcRemember) {
		if ((char *)0 == pcFrom) {
			pcFrom = "localhost";
		}
		strncpy(pcRemember, pcFrom, wMax);
	}
	return fNeg ? -1 : atoi(acPid);
}
%%
#@Explode client@#
%i
static unsigned int uOurCount, uOurSize;
%%
%hi
extern int PTBClientPort(const char *pcDirect), PTBPidSend(int s, char *pcSrc);
%%
%c
/* send our pid to the other process, somehow				(ksb)
 */
int
PTBPidSend(int s, char *pcSrc)
{
	register int iLen;
	auto char acPid[64+MAXHOSTNAMELEN+4];

	if ((char *)0 != pcSrc && strlen(pcSrc) < MAXHOSTNAMELEN) {
		snprintf(acPid, sizeof(acPid), "%ld@\n", (long)getpid());
	} else {
		snprintf(acPid, sizeof(acPid), "%ld\n", (long)getpid());
	}
	iLen = strlen(acPid);
	return iLen == write(s, acPid, iLen) ? 0 : -1;
}

/* The divConnect code has a hook to finish the connection;		(ksb)
 * this is how we finish a ptbw diversion connection.
 * send our pid, as for an 'S' command, read for "iCount,iSize\n"
 */
int
PTBConnHook(int s, void *pvLevel)
{
	register int iFailed, e;
	register unsigned int *pu;
	auto char acMaster[64];

	if ((void *)0 == pvLevel || -1 == s) {
		return s;
	}
	uOurCount = uOurSize = 0;
	pu = & uOurCount;
	iFailed = -1 == PTBPidSend(s, (char *)0) || 1 != write(s, "S", 1);
	if (!iFailed) while (1 == read(s, acMaster, 1)) {
		if ('\000' == acMaster[0] || '\n' == acMaster[0]) {
			break;
		}
		if (',' == acMaster[0]) {
			pu = & uOurSize;
			continue;
		}
		if (!isdigit(acMaster[0])) {
			iFailed = 1;
			continue;
		}
		*pu *= 10;
		*pu += acMaster[0]-'0';
	}
	if (!iFailed) {
		return s;
	}
	e = errno;
	(void)close(s);
	errno = e;
	return -1;
}

/* Make the client port, connect it to the parent server		(ksb)
 * stolen from ptyd, also protected from being passed to a child.
 * Pass pcDirect = /tmp-dir/socket-name to contact
 * a ptbw-like service.
 *
 * We want to keep the hook above from diddling the wrapw socket. So
 * we force the divNextLevel to be a NULL pointer, the divconnect.mc
 * code uses that as a queue to pass a (void *)0 for pvLevel. If you
 * wanted to deference or map it you'd have to code a functor.
 *
 * "But nobody wants to hear this tail, the plot is cliche, the
 *  jokes are stale.  And maybe we've all heard it all before."
 *				Amiee Mann _invisible_ink_
 */
int
PTBClientPort(const char *pcDirect)
{
	register int s;
	auto void *(*pfpvKeep)(void *);
	auto int (*pfiKeep)(int, void *);
	auto char acNotVoid[4];

	pfiKeep = divConnHook;
	divConnHook = PTBConnHook;
	pfpvKeep = divNextLevel;
	divNextLevel = (void *(*)(void *))0;
	/* Now we sent the creds over after we connect, but any
	 * wrapw pvLevel is forced be a (void *)0.
	 */
	s = divConnect(pcDirect, RightsWrap, acNotVoid);
	divConnHook = pfiKeep;
	divNextLevel = pfpvKeep;
	return s;
}
%%
#@Explode tokens@#
%hi
extern char *PTBReadTokenList(int sMaster, unsigned int uNeed, unsigned int **ppuList);
%%
%c
/* Allocate as many tokens as we ask for from the master.		(ksb)
 * We over allocate the memory, but I don't care so much.  We _must_
 * return a valid malloc/free/realloc pointer.
 */
char *
PTBReadTokenList(int sMaster, unsigned int uNeed, unsigned int **ppuList)
{
	register unsigned *puList;
	register unsigned int i, iFailed;
	register char *pcMem, *pcRet;
	auto char acCvt[64];

	if ((unsigned int **)0 != ppuList) {
		*ppuList = (unsigned int *)0;
	}
	if (0 == uNeed) {
		return (char *)0;
	}
	if ((unsigned int *)0 == (puList = calloc(uNeed, sizeof(unsigned int)))) {
		errno = ENOSPC;
		return (char *)0;
	}
	snprintf(acCvt, sizeof(acCvt), "%uG", uNeed);
	write(sMaster, acCvt, strlen(acCvt));
	iFailed = i = 0;
	while (1 == read(sMaster, acCvt, 1)) {
		if ('\n' == acCvt[0]) {
			break;
		}
		if (',' == acCvt[0]) {
			++i;
			continue;
		}
		if ('-' == acCvt[0]) {
			iFailed = 1;
			continue;
		}
		if (isdigit(acCvt[0])) {
			puList[i] *= 10;
			puList[i] += acCvt[0]-'0';
			continue;
		}
#if PTBC_ECHO
		/* stderr for strings that don't look like what we expect
		 */
		(void)write(2, acCvt, 1);
#endif
	}
	if (iFailed || i+1 != uNeed) {
		errno = ENETRESET;
		return (char *)0;
	}

	/* Worst case is we get back the whole token table
	 */
	pcMem = pcRet = malloc((uOurSize|7)+1);
	if ((unsigned int **)0 != ppuList) {
		*ppuList = puList;
	}
	for (i = 0; i < uNeed; ++i) {
		if (0 != i) {
			*pcMem++ = '\n';
		}
		snprintf(acCvt, sizeof(acCvt), "%uN", puList[i]);
		write(sMaster, acCvt, strlen(acCvt));
		if (1 != read(sMaster, pcMem, 1) || '+' != *pcMem) {
			errno = EPROTOTYPE;
			return (char *)0;
		}
		for (; 1 == read(sMaster, pcMem, 1); ++pcMem) {
			if ('\n' == *pcMem) {
				break;
			}
		}
		*pcMem = '\000';
	}
	return pcRet;
}
%%
#@Explode free@#
%hi
extern unsigned int PTBFreeTokenList(int sMaster, unsigned int uHold, unsigned int *puList);
%%
%c
/* Return these tokens back to the master's tablea			(ksb)
 * reply with the number of tokens we still hold (from this list), 0 is Good
 * XXX we could make fewer calls to write(2) here, by a lot. -- ksb
 */
unsigned int
PTBFreeTokenList(int sMaster, unsigned int uHold, unsigned int *puList)
{
	register int uScan, uFailed;
	register int i, iLen, fPanic;
	auto char acCvt[64];

	if ((unsigned int *)0 == puList) {
		return uHold;
	}
	fPanic = 0;
	for (uFailed = uScan = 0; uScan < uHold; ++uScan) {
		if (fPanic)  {
			puList[uFailed++] = puList[uScan];
			continue;
		}
		snprintf(acCvt, sizeof(acCvt), "%uD", puList[uScan]);
		iLen = strlen(acCvt);
		if (iLen != write(sMaster, acCvt, iLen)) {
			fPanic = 1;
			puList[uFailed++] = puList[uScan];
			continue;
		}
		for (i = 0; i < sizeof(acCvt); ++i) {
			if (1 != read(sMaster, acCvt+i, 1)) {
				fPanic = 1;
				puList[uFailed++] = puList[uScan];
				continue;
			}
			if (isspace(acCvt[i])) {
				break;
			}
		}
		acCvt[i] = '\000';
		if (1 != atoi(acCvt)) {
			puList[uFailed++] = puList[uScan];
		}
	}
	return uFailed;
}
%%
#@Explode cmd@#
%hi
extern int PTBCmdInt(int sMaster, int cCmd);
%%
%c
/* Send the master a command, get back an int, or -No, or -1		(ksb)
 */
int
PTBCmdInt(int sMaster, int cCmd)
{
	register int i, c;
	auto char acBuffer[64];

	errno = 0;
	if (sMaster < 0) {
		errno = ENETDOWN;
		return -1;
	}
	if (cCmd < 1 || cCmd > 255) {
		errno = ERANGE;
		return -1;
	}
	acBuffer[0] = cCmd;
	if (1 != write(sMaster, acBuffer, 1)) {
		/* errno from write */
		return -1;
	}
	c = ' ';
	for (i = 0; '\n' != c && i < sizeof(acBuffer); ++i) {
		if (1 != read(sMaster, acBuffer+i, 1)) {
			/* errno from read */
			return -1;
		}
		c = acBuffer[i];
		if (isspace(c)) {
			break;
		}
	}
	if (i == 3 && '-' == acBuffer[0] && 'N' == acBuffer[1] && 'o' == acBuffer[2]) {
		errno = EINVAL;
		return -1;
	}
#if defined(ENOTSUP)
	errno = ENOTSUP;	/* show remote protocol failed as status */
#else
#if defined(EPROTONOSUPPORT)
	errno = EPROTONOSUPPORT;
#else
	errno = ENOPROTOOPT;
#endif
#endif
	acBuffer[i] = '\000';
	return atoi(acBuffer);
}
%%
#@Explode issocket@#
%hi
extern int PTBIsSocket(char *);
%%
%c
/* Is the name maybe a PTBW (or the like) socket			(ksb)
 */
int
PTBIsSocket(char *pcName)
{
	auto struct stat stThis;

	if (-1 == stat(pcName, &stThis))
		return 0;
	return 0 != (S_IFSOCK == (S_IFMT & stThis.st_mode));
}
%%
#@Explode source@#
%hi
extern int PTBReqSource(int sMaster, char *pcSource, size_t iMax, char **ppcMeta);
%%
%c
/* Request the source file for the tokens we are using			(ksb)
 * send "U", get "+path\000" or "-No\000"
 * request the meta information (token comments) in *ppcMeta, when non NULL
 */
int
PTBReqSource(int sMaster, char *pcSource, size_t iMax, char **ppcMeta)
{
	auto char acCheck[2];

	if (1 != write(sMaster, "U", 1)) {
		return -1;
	}
	if (1 != read(sMaster, acCheck, 1)) {
		return -1;
	}
	while (1 == read(sMaster, pcSource, 1) && '\000' != *pcSource++) {
		if (iMax-- < 1)
			break;
	}
	if ((char **)0 != ppcMeta) {
		register int iC = 0;

		*ppcMeta = (char *)0;
		if (-1 != (iC = PTBCmdInt(sMaster, 'C'))) {
			*ppcMeta = calloc((iC|7)+1, sizeof(char));
		}
		if ((char *)0 != *ppcMeta) {
			(void)read(sMaster, *ppcMeta, iC);
		}
	}
	return acCheck[0];
}
%%
#@Explode iota@#
%i
extern char *PTBIota(unsigned int uMax, unsigned int *puWidth);
%%
%c
/* Build the iota (-1) vector						(ksb)
 */
char *
PTBIota(unsigned int uMax, unsigned int *puWidth)
{
	register char *pcCur, *pcMem;
	register unsigned uSize, uCur, uWide, uRem, uBase, uDone;

	uSize = uDone = 0;
	uRem = uMax;
	uBase = 10;
	for (uWide = 1, uCur = 10; uCur < uRem; ++uWide, uCur = uBase-uDone) {
		uSize += uCur*uWide;
		uDone += uCur;
		/* next cycle */
		uRem -= uCur;
		uBase *= 10;
	}
	/* add residual + \000's for string terminators
	 */
	uSize += uRem*uWide + uMax;
	if ((unsigned int *)0 != puWidth) {
		*puWidth = uWide;
	}
	pcCur = pcMem = malloc((uSize|7)+1);
	*pcMem = '\000';
	++uWide;	/* include the '\000' for snprintf below */
	for (uCur = 0; uCur < uMax; ++uCur) {
		snprintf(pcCur, uWide, "%u", uCur);
		pcCur += strlen(pcCur)+1;
	}
	return pcMem;
}
%%
#@Explode quit@#
%hi
extern int PTBQuit(int sMaster, char *pcReply, unsigned uLen);
%%
%c
/* Tell a persistand instance to end					(ksb)
 * When we return -1 we couldn't send it, 0 for not persistant, 1 for OK
 */
int
PTBQuit(int sMaster, char *pcReply, unsigned uLen)
{
	auto char acCvt[256];

	if ((char *)0 == pcReply || 0 == uLen) {
		pcReply = acCvt, uLen = sizeof(acCvt);
	}
	if (1 != write(sMaster, "Q", 1))
		return -1;
	if (0 >= read(sMaster, pcReply, uLen))
		return -1;
	return '-' == pcReply[0] ? 0 : 1;
}
%%
#@Explode pid@#
%hi
extern int PTBReadPid(int sMaster, pid_t *pwMaster);
%%
%c
/* Tell us the pid of the master, via the 'M' command			(ksb)
 * When we return 0 we couldn't get it.
 */
int
PTBReadPid(int sMaster, pid_t *pwMaster)
{
	auto char acCvt[64];

	if (1 != write(sMaster, "M", 1))
		return 0;
	if (0 >= read(sMaster, acCvt, sizeof(acCvt)) || !isdigit(acCvt[0]))
		return 0;
	if ((pid_t *)0 != pwMaster)
		*pwMaster = atoi(acCvt);
	return 1;
}
%%
#@Remove@#
# LLL: We could code a version check against the master, via cmd 'V'
