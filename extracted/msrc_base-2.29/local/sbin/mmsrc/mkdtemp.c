/* $Id: mk.c,v 6.2 2009/12/08 15:15:18 ksb Exp $
 * These functions work like FreeBSD's mktemp and friends, I doubt	(ksb)
 * they are "better", but they give me portable versions I can use.
 * The random pad is pretty random, even if the underlying drand4
 * source lacks entropy.
 */
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>


#if !defined(MKTEMP_SUBST)
#define MKTEMP_SUBST	'X'
#endif

#if !defined(EMU_ARC4RANDOM)
#define EMU_ARC4RANDOM	1
#endif
#if EMU_ARC4RANDOM
#include <time.h>
#include <unistd.h>

/* Nifty function not included in most UNIX libc's			(ksb)
 * This is a stab at something (not nearly as cool), but functional.
 */
static int
arc4random()
{
	static int fInit = 0;

	if (!fInit) {
		auto unsigned short ausSeed[3];
		auto time_t tNow;

		(void)time(& tNow);
		ausSeed[2] ^= tNow & 0xffff;
		ausSeed[1] ^= getpid();
		ausSeed[0] ^= (tNow >> 16) & 0xffff;
		(void)seed48(ausSeed);
		fInit = 1;
	}
	return lrand48();
}
#endif

/* My emulation of mkdtemp(3) from FreeBSD.				(ksb)
 */
static char *
_mktemp(char *pcTempl, int iTail, int (*pfiChk)(char *, void *), void *pvData)
{
	register int i, iLen, iPadLen;
	register char *pcTail, *pcCursor;
	auto char acPad[256];

	if ((char *)0 == pcTempl) {
		errno = EINVAL;
		return pcTempl;
	}
	/* make sure the path is sane, assume "." and "/" exist
	 */
	if ((char *)0 == (pcTail = strrchr(pcTempl, '/'))) {
		pcTail = pcTempl;
	} else {
		auto struct stat stExist;
		*pcTail = '\000';
		if (pcTail != pcTempl && (-1 == stat(pcTempl, &stExist) || !S_ISDIR(stExist.st_mode))) {
			errno = ENOTDIR;
			return (char *)0;
		}
		*pcTail++ = '/';
	}
	if ((char *)0 == (pcCursor = strrchr(pcTail, MKTEMP_SUBST))) {
		errno = ENOENT;
		return (char *)0;
	}
	while (strlen(pcCursor) < iTail || MKTEMP_SUBST != *pcCursor) {
		if (pcTail == pcCursor) {
			errno = ENOENT;
			return (char *)0;
		}
		--pcCursor;
	}
	/* Build a map of characters to use, more the better, and anti-sort
	 * them to boot (ksb@mail.npcguild.org)
	 */
	iPadLen = 0;
	for (i = '0'; i <= '9'; ++i)
		acPad[iPadLen++] = i;
	if (arc4random() & 1)
		acPad[iPadLen++] = '@';
	for (i = 'a'; i <= 'z'; ++i)
		acPad[iPadLen++] = i;
	if (arc4random() & 2)
		acPad[iPadLen++] = '_';
	for (i = 'A'; i <= 'Z'; ++i)
		acPad[iPadLen++] = i;
	if (arc4random() & 4)
		acPad[iPadLen++] = ',';
	for (i = iPadLen*2/3; i > 0; i /= 2) {
		for (iLen = 0; iLen+i < iPadLen; ++iLen) {
			register int iSwap;
			if (arc4random() & 8)
				continue;
			iSwap = acPad[iLen];
			acPad[iLen] = acPad[iLen+i];
			acPad[iLen+i] = iSwap;
		}
	}

	/* We'll take a lot more than 6 X's
	 */
	iLen = 0;
	do {
		++iLen;
		*pcCursor = acPad[arc4random() % iPadLen];
		if (pcCursor == pcTail)
			break;
		--pcCursor;
	} while (MKTEMP_SUBST == pcCursor[0]);

	for (i = 0; 0 == (*pfiChk)(pcTempl, pvData); i = (i+1) % iLen) {
		if (EEXIST != errno)
			return (char *)0;
		pcCursor[i] = acPad[arc4random() % iPadLen];
	}
	return pcTempl;
}

/* function for above to look for a file by opening it			(ksb)
 */
static int
_chk_stemp(char *pcTry, void *pvFd)
{
	register int *piFd = (int *)pvFd;

	*piFd = open(pcTry, O_CREAT|O_EXCL|O_RDWR, 0600);
	return -1 == *piFd ? 0 : 1;
}

/* function to look for a directory by making it			(ksb)
 */
static int
_chk_dtemp(char *pcTry, void *pvNull)	/*ARGSUSED*/
{
	return 0 == mkdir(pcTry, 0700);
}


/* emulate mkdtemp							(ksb)
 */
char *
mkdtemp(char *pcTempl)
{
	return _mktemp(pcTempl, 0, _chk_dtemp, (void *)0);
}

