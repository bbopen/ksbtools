/* $Id: util_editor.mc,v 8.10 1998/09/16 13:40:15 ksb Exp $
 * find the User's favorite editor and run it on the named file		(ksb)
 */
int
%%K<util_editor>1v(u_pcFile)
char *u_pcFile;
{
	register pid_t wPid;
	auto char *pcTail, *pcEditor;
	auto int iStatus;
	static char *apcEditors[] = {
%		%K<util_editor_path>/ql,
		(char *)0
	};
	static char *apcRun[4] = {
		(char *)0, (char *)0, (char *)0, (char *)0
	};
	extern char **environ;

%	pcEditor = %K<util_fsearch>1v(apcEditors, getenv("PATH"))%;
	if ((char *)0 == pcEditor) {
		pcEditor = BIN_vi;
	}
	if ((char *)0 == (pcTail = strrchr(pcEditor, '/'))) {
		pcTail = pcEditor;
	} else {
		++pcTail;
	}
	apcRun[0] = pcTail;
	apcRun[1] = u_pcFile;
	apcRun[2] = (char *)0;

	switch (wPid = fork()) {
	case -1:
		return -1;
	case 0:
		execve(pcEditor, apcRun, environ);
		execve(pcEditor, apcRun, (char **)0);
%		fprintf(stderr, "%%s: %%s: %%s\n", %b, pcEditor, %E)%;
		exit(8);
	default:
		break;
	}
	if (-1 == waitpid(wPid, &iStatus, 0)) {
		return 1;
	}
	if (WIFEXITED(iStatus)) {
		return WEXITSTATUS(iStatus);
	}
	return WIFSIGNALED(iStatus);
}
