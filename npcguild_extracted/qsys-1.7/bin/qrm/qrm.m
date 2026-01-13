#!mkcmd
# qrm [-nv] [-d Q] jids
# $Compile(*): %b %o -mMkcmd %f && %b %o prog.c
# $Mkcmd(*): ${mkcmd-mkcmd} std_help.m std_version.m ../common/qsys.m %f

%i
#include <sys/time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>

#include "common.h"

static char rcsid[] =
	"$Id: qrm.m,v 1.2 1994/02/09 15:02:21 ksb Exp $";
%%

basename "qrm" ""


integer variable "fd" {
	hidden
}

integer variable "iErrs" {
	hidden init "0"
}

after {
	update "fd = LockQ(%rdn);"
}

every {
	named "QDel"
	param "jids"
	help "job identifiers to remove from the queue"
}

zero {
	update 'fprintf(stderr, "%%s: must provide job ids to remove\\n", %b);'
	abort "exit(3);"
}

exit {
	update "UnLockQ(fd);"
	abort "exit(iErrs);"
}

%c
extern char *getenv(), *mktemp(), *getlogin();
extern char *ctime();
extern int errno;
#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif

#if defined(SYSV) || defined(HPUX7) || defined(HPUX8) || defined(IBMR2)
#define getwd(Mp) getcwd(Mp,MAXPATHLEN) /* return pwd           */
extern char *getcwd();
#else
extern char *getwd();
#endif

#include "qfuncs.i"

/* All digits in a string?						(ksb)
 */
int
AllDigits(pc)
register char *pc;
{
	while (isdigit(*pc))
		++pc;
	return '\000' == *pc;
}

/* handle the case of a running jobs					(ksb)
 */
QKill(iPid, pQ)
int iPid;
Q *pQ;
{
	register int i;
	auto char acCmd[132];
	auto char acClue[1024];
	auto char acOn[256], acUs[256];
	register FILE *fpClue;

	sprintf(acClue, "%s/clue_%d", pcQDir, iPid);

	acOn[0] = '\000';
	if ((FILE *)0 != (fpClue = fopen(acClue, "r")) && 0 == gethostname(acUs, sizeof(acUs))) {
		register int c, i, cFirst;

		do {
			cFirst = getc(fpClue);
		} while (EOF != cFirst && isspace(cFirst));

		i = 1;
		while (EOF != (c = getc(fpClue)) && !isspace(c)) {
			acOn[i++] = c;
		}
		acOn[i] = '\000';
		if (cFirst != acUs[0] || 0 != strcmp(acUs+1, acOn+1)) {
			acOn[0] = cFirst;
		}
	}
	if ((FILE *)0 != fpClue) {
		fclose(fpClue);
	}

	if ('\000' != acOn[0]) {
		sprintf(acCmd, "%s %s -n /usr/local/etc/qkill -15%s %d", PATH_RSH, acOn, fVerbose ? " -v" : "", iPid);
	} else {
		sprintf(acCmd, "/usr/local/etc/qkill -15%s %d", fVerbose ? " -v" : "", iPid);
	}

	if (fVerbose) {
		printf("%s: %s\n", progname, acCmd);
	}

	for (i = 0; i < pQ->irun; ++i) {
		register char *pcName;

		pcName = pQ->ppDErun[i]->d_name;
		if (atoi(pcName+4) != iPid) {
			continue;
		}
		FreeQ(pQ);
		return 0 == system(acCmd);
	}
	fprintf(stderr, "%s: %s: %d: no such process in queue\n", progname, pcQDir, iPid);
	return 1;
}

/* output a nifty table of the queue'd jobs				(ksb)
 */
static void
QDel(pcJid)
char *pcJid;
{
	register int i;
	auto Q QCur;
	auto char acCmd[4096];

	ScanQ(pcQDir, & QCur);

	if (0 == QCur.ijobs && 0 == QCur.irun) {
		printf("zero files in queue\n");
		exit(1);
	}

	if (isalpha(pcJid[0]) && isdigit(pcJid[1]) && isdigit(pcJid[2]) && '\000' == pcJid[3]) {
		/* classic jid */;
	} else if (AllDigits(pcJid)) {
		QKill(atoi(pcJid), & QCur);
		return;
	} else {
		fprintf(stderr, "%s: `%s\': bad job id -- must be <letter><digit><digit>\n", progname, pcJid);
		exit(2);
	}
	if (isupper(*pcJid)) {
		*pcJid = tolower(*pcJid);
	}

	for (i = 0; i < QCur.ijobs; ++i) {
		register char *pcName;
		register int iWho;
		auto struct stat stCheck;
		auto char acPath[MAXPATHLEN+1];

		pcName = QCur.ppDElist[i]->d_name;
		if (pcName[0] != pcJid[0] || pcName[1] != pcJid[1] || pcName[2] != pcJid[2]) {
			continue;
		}

		/* we have the one we want...
		 */
		sprintf(acPath, "%s/%s", pcQDir, pcName);
		if (-1 == stat(acPath, & stCheck)) {
			fprintf(stderr, "%s: stat: %s: %s\n", progname, acPath, strerror(errno));
			++iErrs;
			break;
		}
		iWho = getuid();
		if (0 != iWho && iWho != stCheck.st_uid) {
			fprintf(stderr, "%s: %s: Permission denied\n", progname, pcName);
			++iErrs;
			break;
		}
		if (fVerbose) {
			printf("%s: rm -f %s\n", progname, acPath);
		}
		if (fExec) {
			if (-1 == unlink(acPath)) {
				fprintf(stderr, "%s: unlink: %s: %s\n", progname, acPath, strerror(errno));
				++iErrs;
			}
		}
		break;
	}
	if (i == QCur.ijobs) {
		fprintf(stderr, "%s: %s: no such job in queue %s\n", progname, pcJid, pcQDir);
		++iErrs;
	}
	FreeQ(& QCur);
}
