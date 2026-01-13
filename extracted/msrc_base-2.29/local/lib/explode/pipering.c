/*@Version $Revision: 1.9 $@*/
/*@Header@*/
/* pipe ring implementation -- ksb
 * $Id: pipering.c,v 1.9 2009/10/16 15:47:42 ksb Exp $
 *
 * A pipe ring is a group of processes that form a normal pipeline, with
 * the output of the last (right most) process directed back to the
 * input of the first (left most).  This is pretty bad (but not impossible)
 * to do in a shell script.  It is also VERY useful for sequential parallel
 * processing.
 *
 * For a test driver compile this file with -DTEST.
 */
/*@Version $Revision: 1.9 $@*/
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sysexits.h>

extern char *progname;

/*@Explode ring@*/
/* start iCount workers in a pipe-ring.  The worker function is called	(ksb)
 * with a worker number (0 .. iCount-1) the input Fd, output Fd, and the
 * data pointer:
 *	int
 *	PipeWorker(int iWorker, int fdIn, int fdOut, void *pvData)
 *	{ .... }
 *
 * You provide a "start" token, to inject in the pipe-ring, or we send "go\n".
 *
 * We return 0 for run, -1 for failed.  piRet[0...iCount-1] has the return
 * codes from the worker functions, if it's not (int *)0.
 */
int
PipeRing(pfiWorker, iCount, pvData, piRet, pcStart)
int (*pfiWorker)();
int iCount, *piRet;
void *pvData;
char *pcStart;
{
	register int i, iPid, *piRec, iRet;
	auto int fdWrite, fdRead, fdKick, afdRing[2];

	/* Older stdio's don't know about this, it'll drop core pre C89
	 */
#if defined(__stdc__) || defined(__STRICT_ANSI__)
	fflush((FILE *)0);
#else
	fflush(stdout), fflush(stderr);
#endif
	if (0 == iCount) {
		return 0;
	}

	piRec = (int *)calloc(iCount, sizeof(int));
	if (-1 == pipe(afdRing)) {
		return -1;
	}
	fdRead = afdRing[0];
	fdKick = afdRing[1];
	iRet = 0;
	for (i = 0; i < iCount; ++i) {
		register int wFib1, wFib2, wTemp;

		if (i+1 == iCount) {
			afdRing[0] = -1;
			afdRing[1] = -1;
			fdWrite = fdKick;
		} else if (-1 == pipe(afdRing)) {
			/* Oh, my! without a pipe we fail.  So by closing
			 * the input side of the last pipe we make all the
			 * worker's reads return 0, please abort (quietly).
			 */
			fprintf(stderr, "%s: pipe: %s\n", progname, strerror(errno));
			iRet = -1;
			iCount = i;
			goto cleanup;
		} else {
			fdWrite = afdRing[1];
		}
		wFib1 = wFib2 = 1;
		for (;;) { switch (iPid = fork()) {
		case 0:	/* child, close what we don't want */
			if (-1 != afdRing[0])
				close(afdRing[0]);
			if (fdWrite != fdKick)
				close(fdKick);
			exit(pfiWorker(i, fdRead, fdWrite, pvData));
			_exit(EX_SOFTWARE);
		case -1:
			if (EAGAIN == errno && PIPERING_STALL >= wFib2) {
				sleep(wFib2);
				wTemp = wFib2;
				wFib2 += wFib1;
				wFib1 = wTemp;
				continue;
			}
			/* If this is the last kid we can substitute
			 * ourself, if we knew the worker function didn't
			 * execve some other process (we don't for sure).
			 */
			fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
			if (-1 != afdRing[0] && afdRing[0] != fdRead)
				(void)close(afdRing[0]);
			if (-1 != afdRing[1])
				(void)close(afdRing[1]);
			(void)close(fdRead);
			iRet = -1;
			iCount = i;
			goto cleanup;
		default:
			if ((int *)0 != piRec) {
				piRec[i] = iPid;
			}
			break;
		} break; }
		if (fdWrite != fdKick) {
			close(fdWrite);
		}
		close(fdRead);
		fdRead = afdRing[0];
	}

	/* Let them run, start with the User's start token
	 */
	if ((char *)0 == pcStart) {
		pcStart = "go\n";
	}
	write(fdKick, pcStart, strlen(pcStart));
 cleanup:
	close(fdKick);
	close(fdRead);

	/* If we could not keep the record of the pid at least wait
	 * for the correct number of children.  We could allow for
	 * an arbitrary exit order, but they should normally exit in
	 * the sequence they were started (think about it). -- ksb
	 */
	for (i = 0; i < iCount; ++i) {
		auto int iCode;

		if ((int *)0 == piRec)
			wait((void *)& iCode);
		else
			waitpid(piRec[i], (void *)&iCode, 0);
		if ((int *)0 != piRet)
			piRet[i] = iCode;
	}
	return iRet;
}

/*@Explode pump@*/
/* push data from stdin to stdout in a pipe-ring task force		(ksb)
 * we send two tokens around the ring "r" and "w"
 * when we read a "r" we can read the next block, when we read the
 * "w" we can write what we read (last time).  If we read EOF for
 * the "r" we close the pipe and exit happy, for the "w" we close
 * and exit sad (EX_SOFTWARE).
 *
 * The block we get in buffer size to read/write (int), default 20b
 * (what else) because tar, cpio, and dd all default to 20b (10k)
 *
 *	PipeRing(PipePump, iWorkers, (void *)& iBytes, piRet, "rw");
 *
 * XXX we should take an option to pack output records or not.  This would
 * allow a device that returns 2b blocks to pack up 10 reads into a 20b
 * write block.  We don't "speed match" now.			(ksb)
 *
 * N.B.:  iWorkers should be > 1, but 1 works -- it's just slow.
 */
int	/*ARGSUSED*/
PipePump(iWorker, fdIn, fdOut, pvData)
int iWorker;
int fdIn, fdOut;
void *pvData;
{
	register int iBufSize;
	register char *pcBuf;
	register int iCC, iOut, iPos;
	auto char acToken[4];	/* 'r' or 'w' */

	iBufSize = (void *)0 == pvData ? 10240 : *(int *)pvData;
	pcBuf = malloc(iBufSize);
	if ((char *)0 == pcBuf) {
		exit(EX_OSERR);	/* no mem */
	}
	/* wait for the read token ('r')
	 * read some data, tell the next one to read (unless we saw EOF)
	 * wait for the write token ('w')
	 * write the data we read, maybe multiple writes
	 * tell the next guy to write
	 */
	while (1 == (iCC = read(fdIn, acToken, 1)) && 'r' == acToken[0]) {
		iCC = read(0, pcBuf, iBufSize);
		if (0 == iCC) {
			/* accept the write token to avoid sigpipe in n-1
			 * but ignore the value
			 */
			(void)read(fdIn, acToken, 1);
			break;
		}
		write(fdOut, "r", 1);
		if (1 != read(fdIn, acToken, 1) || 'w' != acToken[0]) {
			break;
		}
		iPos = 0;
		while (0 != iCC && -1 != (iOut = write(1, pcBuf+iPos, iCC))) {
			iPos += iOut;
			iCC -= iOut;
		}
		write(fdOut, "w", 1);
	}
	exit(0 == iCC ? EX_OK : EX_SOFTWARE);
}

/*@Explode main@*/
#if TEST
char *progname = "pipe test";

/* Example worker in a pipe-ring					(ksb)
 * We hold the initial token if we are worker #0;
 * our task might be defined but pvData[iWorker], or by the input token
 *
 * This is a pretty limitted worker, the token size of 32 is geared
 * for a small integer.  We just echo the start token around the ring.
 */
static int
Worker1(iWorker, fdIn, fdOut, pvData)
int iWorker;
int fdIn, fdOut;
void *pvData;
{
	register int i;
	register char *pcData;
	auto char acToken[32];

	pcData = ((char **)pvData)[iWorker];

	/* keep going while we get a token
	 */
	while (0 < (i = read(fdIn, acToken, sizeof(acToken)))) {
		if ('\000' != *pcData) {
			(void)write(1, pcData++, 1);
		}
		if ('\000' == *pcData) {
			break;
		}
		write(fdOut, acToken, i);
	}
	(void)write(1, pcData, strlen(pcData));
	close(fdOut);
	close(fdIn);
	return 0;
}

/* another test example							(ksb)
 */
static int
Worker2(iWorker, fdIn, fdOut, pvData)
int iWorker;
int fdIn, fdOut;
void *pvData;
{
	auto char acToken[80], acSend[32];
	register int iLoop, cc;

	iLoop = ((void *)0 == pvData) ? 1 : *(int *)pvData;

	while (iLoop-- > 0) {
		sprintf(acSend, "%d.%d", iWorker, iLoop);
		cc = read(fdIn, acToken, sizeof(acToken));
		if (1 > cc) {
			printf("%s: %d: read EOF (%d)\n", progname, iWorker, cc);
			exit(EX_PROTOCOL);
		}
		if (cc < sizeof(acToken)) {
			acToken[cc] = '\000';
		}
		printf("%s: %d: read token \"%s\", writing \"%s\"\n", progname, iWorker, acToken, acSend);
		/* the last worker gets a SIGPIPE (13) here -- ksb
		 */
		write(fdOut, acSend, strlen(acSend));
	}
	exit(EX_OK);
}


/* Show how the pipe ring works, at least a little			(ksb)
 */
int
main(argc, argv)
int argc;
char **argv;
{
	static char *apcWords[] = {
		"Hlwl",
		"eood \nNice to see you",
		"l r\n",
		"uncommon"
	};
	auto int aiRet[6];
	auto int iLoop, iBytes, *piRet;

	PipeRing(Worker1, 3, (void *)apcWords, (int *)0, (char *)0);

	iLoop = 2;
	PipeRing(Worker2, sizeof(aiRet)/sizeof(aiRet[0]), (void *)& iLoop, aiRet, "first");
	for (iLoop = 0; iLoop < sizeof(aiRet)/sizeof(aiRet[0]); ++iLoop) {
		printf(" worker%d return %d\n", iLoop, aiRet[iLoop]);
	}

	iLoop = 4, iBytes = 10;
	piRet = calloc(iLoop, sizeof(int));
	printf("Data pump test, %d byte%s per cycle, %d workers (looking for EOF):\n", iBytes, 1 == iBytes ? "" : "s", iLoop);
	PipeRing(PipePump, iLoop, (void *)& iBytes, piRet, "rw");
	iBytes = iLoop;
	for (iLoop = 0; iLoop < iBytes; ++iLoop) {
		printf(" worker%d return %d\n", iLoop, piRet[iLoop]);
	}

	exit(EX_OK);
}
#endif
