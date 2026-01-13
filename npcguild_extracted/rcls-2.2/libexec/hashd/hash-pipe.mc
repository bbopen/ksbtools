/* $Id: hash-pipe.mc,v 2.2 2000/06/14 01:28:22 ksb Exp $
 *
 * rather than doing the locking ourself open a process to		(ksb)
 * lock the sequence and update it, just read the hash from the pipe.
 */
static int
PipeHashd(char *pcSeq)
{
	auto int aiFd[2];
	extern char **environ;
	static char acPadLen[32];
	static char *apcNewArgv[] = {
		"hashd", "-P", "-p", acPadLen, "/var/run/hash.seq",
#define HASH_SEQ_PATH				4 /* ^^^ this string */
		(char *)0
	};

	if (-1 == pipe(aiFd)) {
%		fprintf(stderr, "%%s: pipe: %%s\n", %b, strerror(errno))%;
		return -1;
	}
	switch (fork()) {
	case -1:
		close(aiFd[0]);
		close(aiFd[1]);
		return -1;
	default:
		close(aiFd[1]);
		return aiFd[0];
	case 0:
		break;
	}
	if (1 != aiFd[1]) {
		close(1);
		dup(aiFd[1]);
		close(aiFd[1]);
	}
	if (1 != aiFd[0]) {
		close(aiFd[0]);
	}
	if ((char *)0 != pcSeq) {
		apcNewArgv[HASH_SEQ_PATH] = pcSeq;
	}
%	sprintf(acPadLen, "%%d", %K<hash_pipe_size>1v)%;
%	execve(%K<hashd_path>1eqv, apcNewArgv, environ)%;
	write(1, "\000", 1);
	exit(1);
}

/* read a new hash from the pipe					(ksb)
 * If we get a null one (leading '\000') close the pipe and stop trying
 * (a re-init call will start again).
 */
char *
NewHash(char *pcHash)
{
	if (-1 == fdHashPipe) {
		return (char *)0;
	}
	if (HASH_LENGTH+1 != read(fdHashPipe, pcHash, HASH_LENGTH+1) || '\000' == *pcHash) {
		close(fdHashPipe);
		fdHashPipe = -1;
		return (char *)0;
	}
	return pcHash;
}
