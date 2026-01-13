/* $Id: util_pipefit.mc,v 8.40 2007/09/17 18:59:54 ksb Exp $
 * open a process on 0 or 1 (or 2) close the old fd			(ksb)
 * replace it with a pipe to the new process in the correct dir.
 *
 * const char *pcExec, char **ppcArgv, char *ppcEnv, int iFd
 */
static pid_t
%%K<PipeFit>/ef
{
	extern char **environ;
	register int i, iKeep, iPid;
	auto int aiPipe[2];

	/* iKeep is the subscript in the pipe array to install at iFd,
	 * in this process, note iFd could be 2 (for stderr).
	 */
	iKeep = (0 == iFd ? 0 : 1);
%	if (fVerbose && (FILE *)0 != %K<PipeVerbose>1ev) %{
%		fprintf(%K<PipeVerbose>1ev, "%%s:%%s", %b, iKeep ? " |" : "")%;
		for (i = 0; (char *)0 != ppcArgv[i]; ++i) {
%			fprintf(%K<PipeVerbose>1ev, " %%s", ppcArgv[i])%;
		}
%		fprintf(%K<PipeVerbose>1ev, "%%s\n", iKeep ? "" : " |")%;
%		fflush(%K<PipeVerbose>1ev)%;
	}
	if (-1 == iFd || 0 == fExec) {
		return 0;
	}
	if (-1 == pipe(aiPipe)) {
%		fprintf(stderr, "%%s: pipe: %%s\n", %b, strerror(errno))%;
		return -1;
	}
	switch (iPid = fork()) {
	case -1:
		close(aiPipe[0]);
		close(aiPipe[1]);
%		fprintf(stderr, "%%s: fork: %%s\n", %b, strerror(errno))%;
		return -1;
	default:	/* parent , close fd, teather kid up */
		break;
	case 0:
		/* child inverts the logic
		 */
		iFd = 0 == iFd ? 1 : 0;
		iKeep ^= 1;
		break;
	}
	if (iFd != aiPipe[iKeep]) {
		dup2(aiPipe[iKeep], iFd);
		close(aiPipe[iKeep]);
	}
	if (iFd != aiPipe[!iKeep]) {
		close(aiPipe[!iKeep]);
	}
	clearerr(0 == iFd ? stdin : 1 == iFd ? stdout : stderr);
	if (0 != iPid) {
		return iPid;
	}
	environ = ppcEnv;
	(void)execvp(pcExec, ppcArgv);
	(void)execve(pcExec, ppcArgv, ppcEnv);
%	fprintf(stderr, "%%s: execve: %%s: %%s\n", %b, pcExec, strerror(errno))%;
	exit(EX_UNAVAILABLE);
}
