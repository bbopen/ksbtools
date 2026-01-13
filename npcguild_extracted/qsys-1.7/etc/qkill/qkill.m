#!mkcmd
# $Compile(*): ${mkcmd-mkcmd} -n main std_help.m std_version.m %f && %b %o main.c
from '<sys/time.h>'
from '<sys/file.h>'
from '<sys/types.h>'
from '<sys/stat.h>'
from '<sys/errno.h>'
from '<syslog.h>'
from '<signal.h>'
from '<errno.h>'

from '<sys/param.h>'
from '<machine/vmparam.h>'
from '<sys/proc.h>'
from '<nlist.h>'

from '"main.h"'
from '"common.h"'

%h
extern char acKmem[];
%%


%i
static char rcsid[] =
	"$Id: qkill.m,v 1.2 1997/01/06 01:49:13 kb207252 Exp $";
%%

%c
char
	acKmem[] = "/dev/kmem";

extern int errno;
#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif


struct nlist nl[] = {
	{ "_proc" },
#define KPROC			0
	{ "_nproc" },
#define KNPROC			1
	{ "" }
};
static int fdKmem;

/* find the kernel for the current machine				(ksb)
 */
static void
FindUnix()
{
	register char **ppc;
	register int fd;
	static char *apcUnix[] = {
		"/unix",
		"/vmunix",
		"/dynix",
		"/hp-ux",
		"/386bsd",
		"/kernel",
		(char *)0
	};

	if ((char *)0 != pcUnix)
		return;
	for (ppc = apcUnix; (char *)0 != *ppc; ++ppc) {
		if (-1 != (fd = open(*ppc, 0, 0))) {
			pcUnix = *ppc;
			(void)close(fd);
			break;
		}
	}
	if ((char *)0 == pcUnix) {
		fprintf(stderr, "%s: cannot find UNIX kernel\n", progname);
		exit(1);
	}
	if (-1 == (fdKmem = open(pcKmem, 0, 0))) {
		fprintf(stderr, "%s: open: %s: %s\n", progname, acKmem, strerror(errno));
		exit(1);
	}
	nlist(pcUnix, nl);
	if (nl[0].n_type==0) {
		fprintf(stderr, "%s: %s: no namelist\n", progname, pcUnix);
		exit(1);
	}
}
%%
before {
	named "FindUnix"
	update "%n();"
}


boolean 'v' {
	named "fVerbose"
	help "output actions as shell commands"
}
boolean 'n' {
	named "fExec"
	init "1"
	help "do not really kill any processes"
}
char* 'K' {
	hidden named "pcKmem"
	init "acKmem"
	help "name of the kernel memory interface device"
}
char* 'U' {
	hidden named "pcUnix"
	help "name of the running kernel"
}
number {
	named "iSig"
	param "sig"
	help "signal to send (no symbolics)"
}
every integer {
	local named "iPid"
	user "QKill(%n);"
	param "pids"
	help "process ids to cleanup"
}

%c
struct proc *procbase;
int iProcs;

static void
CKill(iPPid, pProc)
int iPPid;
struct proc *pProc;
{
	register int i, iFound;
	auto struct proc *proc, pthis;

	proc = procbase;
	iFound = -1;
	for (i = 0; i < iProcs; ++proc, ++i) {
		(void)lseek(fdKmem, (long)proc, 0);
		(void)read(fdKmem, (char *)& pthis, sizeof(pthis));
		if (0 == pthis.p_stat) {
			continue;
		}

#if USE_p_pptr
		if (pProc != pthis.p_pptr) {
			continue;
		}
#else
		if (iPPid != pthis.p_ppid) {
			continue;
		}
#endif
		CKill(pthis.p_pid, proc);
	}
	if (fVerbose) {
		printf("kill -%d %d\n", iSig, iPPid);
	}
	if (!fExec) {
		return;
	}
	if (-1 == kill(iPPid, iSig) && ESRCH != errno) {
		fprintf(stderr, "%s: kill: %d: %s\n", progname, iPPid, strerror(errno));
	}
}

static int
QKill(iPid)
int iPid;
{
	register int i, iFound;
	auto struct proc *proc, pthis;

	(void)lseek(fdKmem, (long)nl[KNPROC].n_value, 0);
	(void)read(fdKmem, (char *)& iProcs, sizeof(iProcs));

	(void)lseek(fdKmem, (long)nl[KPROC].n_value, 0);
	(void)read(fdKmem, (char *)& procbase, sizeof(proc));

	proc = procbase;

	iFound = -1;
	for (i = 0; i < iProcs; ++proc, ++i) {
		(void)lseek(fdKmem, (long)proc, 0);
		(void)read(fdKmem, (char *)& pthis, sizeof(pthis));
		if (0 == pthis.p_stat) {
			continue;
		}

		if (iPid == pthis.p_pid) {
			iFound = i;
			break;
		}
	}
	if (-1 == iFound) {
		fprintf(stderr, "%s: pid %d: not found\n", progname, iPid);
		return 1;
	}

	if (0 == getuid()) {
		/* don't check anything */
	} else {
#if USE_pcred
		auto struct pcred MYcred;
		(void)lseek(fdKmem, (long)pthis.p_cred, 0);
		if (sizeof(MYcred) != read(fdKmem, (char *)& MYcred, sizeof(MYcred))) {
			fprintf(stderr, "%s: %d: no credentials?\n", progname, iPid);
			return 1;
		}
		if (getuid() != MYcred.p_ruid) {
			fprintf(stderr, "%s: %d: not owner\n", progname, iPid);
			return 1;
		}
#else
		if (getuid() != pthis.p_uid) {
			fprintf(stderr, "%s: %d: not owner\n", progname, iPid);
			return 1;
		}
#endif
	}

	CKill(pthis.p_pid, proc);
	return 0;
}
%%

exit {
	update "close(fdKmem);"
}
