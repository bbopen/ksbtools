/* $Id: qfuncs.i,v 1.3 1997/01/05 20:54:05 kb207252 Exp $
 */
#if HAS_NDIR
#include <ndir.h>
#else
#if HAS_DIRENT
#if defined(NETBSD)
#include <dirent.h>
#else
#include <sys/dirent.h>
#endif
#else
#include <sys/dir.h>
#endif
#endif

#if USE_LOCKF
#include <unistd.h>
#endif


typedef struct Qnode {
	unsigned irun;			/* jobs that might be running	*/
	unsigned iuser;			/* number of single thread user	*/
	unsigned ijobs;			/* jobs in the wait queue	*/
	struct DIRENT **ppDErun;	/* list of running jobs		*/
	struct DIRENT **ppDEuser;	/* list of running users	*/
	struct DIRENT **ppDElist;	/* list of waiting jobs		*/
} Q;

static char acProtoLock[] = ".qlock";
static char acProtoRun[] = "run_%d";
static char acProtoClue[] = "clue_%d";
static char acProtoUser[] = "uid_%d";

/* hold the queue steady while we look at it, please			(ksb)
 */
static int
LockQ(pcDir)
char *pcDir;
{
	register int fd;
	auto char acLock[MAXPATHLEN];

	sprintf(acLock, "%s/%s", pcDir, acProtoLock);

	if (-1 == (fd = open(acLock, O_RDWR, 0664))) {
		fprintf(stderr, "%s: open: %s: %s\n", progname, acLock, strerror(errno));
		exit(16);
	}
	(void)fcntl(fd, F_SETFD, 1);
#if USE_FLOCK
	while (-1 == flock(fd, LOCK_EX|LOCK_NB)) {
		register int cc;
		auto char acPid[100];

		if (errno != EWOULDBLOCK) {
			fprintf(stderr, "%s: flock: %d: %s\n", progname, fd, strerror(errno));
			exit(16);
		}
		(void)lseek(fd, 0L, 0);
		if ((cc = read(fd, acPid, sizeof acPid)) < 2 || (char *)0 == strchr(acPid, '\n')) {
			continue;
		}
		if (! isdigit(acPid[0])) {
			do {
				write(2, acPid, cc);
			} while (0 < (cc = read(fd, acPid, sizeof acPid)));
		} else {
			sprintf(acLock, "%s: waiting for process %s", progname, acPid);
			write(2, acLock, strlen(acLock));
		}
		if (-1 == flock(fd, LOCK_EX)) {
			fprintf(stderr, "%s: flock: %d: %s\n", progname, fd, strerror(errno));
			exit(16);
		}
		break;
	}
#else /* use lockf -- yucko */
	while (-1 == lockf(fd, F_TLOCK, 0L)) {
		register int cc;
		auto char acPid[100];

		if (errno != EAGAIN && errno != EACCES) {
			fprintf(stderr, "%s: lockf: %d: %s\n", progname, fd, strerror(errno));
			exit(16);
		}
		(void)lseek(fd, 0L, 0);
		if ((cc = read(fd, acPid, sizeof acPid)) < 2 || (char *)0 == strchr(acPid, '\n')) {
			continue;
		}
		if (! isdigit(acPid[0])) {
			do {
				write(2, acPid, cc);
			} while (0 < (cc = read(fd, acPid, sizeof acPid)));
		} else {
			sprintf(acLock, "%s: waiting for process %s", progname, acPid);
			write(2, acLock, strlen(acLock));
		}
		if (-1 == lockf(fd, F_LOCK, 0L)) {
			fprintf(stderr, "%s: lockf: %d: %s\n", progname, fd, strerror(errno));
			exit(16);
		}
		break;
	}
#endif
	(void)lseek(fd, 0L, 0);
	sprintf(acLock, "%d\n", getpid());
	write(fd, acLock, strlen(acLock));
	return fd;
}

/* release the procs waiting for the job queue				(ksb)
 */
static void
UnLockQ(fd)
int fd;
{
	(void)ftruncate(fd, 0L);
#if USE_FLOCK
	(void)flock(fd, LOCK_UN);
#else /* use lockf */
	(void)lockf(fd, F_ULOCK, 0L);
#endif
	close(fd);
}

/* jobs in the queue start with a letter (a-zA-Z)			(ksb)
 * and have prio (-20 to +20, and id encoded) jobs of the
 * same prio have .seq appended
 * So format is iii-dd.seq or iii+dd.seq.
 * running jobs are run_%d (%d is the pid, BTW)
 * users who want only one job at a time to run have a lock file
 * `uid_%d' which contains the pid of the job holding the lock
 * (%d is the uid, BTW).
 */
static int
QFile(pDE)
struct DIRENT *pDE;
{
	register char *pc;

	pc = pDE->d_name;
	if (isalpha(pc[0]) && '\000' != pc[1] && '\000' != pc[2]) {
		if ('-' == pc[3] || '+' == pc[3] || '_' == pc[3]) {
			return 1;
		}
	}
	return 0;
}

/* put the jobs in prio order, used by both qmgr and qstat		(ksb)
 */
static int
QfSort(d1, d2)
struct DIRENT **d1, **d2;
{
	register char *pcl, *pcr;
	register int l, r;

	pcl = (*d1)->d_name;
	pcr = (*d2)->d_name;
	if ('_' == pcl[2]) {		/* run_%d files are first */
		if ('_' != pcr[2])
			return 1;
	} if ('_' == pcr[2]) {
		return -1;
	}
	l = atoi(pcl+3);
	r = atoi(pcr+3);
	if (l != r)
		return l - r;
	l = atoi(pcl+7);
	r = atoi(pcr+7);
	if (l != r)
		return l - r;
	return strcmp(pcl, pcr);
}


/* get an in core copy of the queue -- if it is not locked we might	(ksb)
 * not get a clean one.
 */
void
ScanQ(pcDir, pQ)
char *pcDir;
Q *pQ;
{
	register int n;
	auto struct DIRENT **ppDEList;

	pQ->ijobs = 0;
	if (-1 == (n = scandir(pcDir, &ppDEList, QFile, QfSort))) {
		return;
	}
	pQ->ppDErun = ppDEList;
	for (pQ->irun = 0; n > 0; ++(pQ->irun), --n, ++ppDEList) {
		register char *pc;
		pc = ppDEList[0]->d_name;
		if ('_' != pc[3] || 'r' != pc[0])
			break;
	}
	pQ->ppDEuser = ppDEList;
	for (pQ->iuser = 0; n > 0; ++(pQ->iuser), --n, ++ppDEList) {
		register char *pc;
		pc = ppDEList[0]->d_name;
		if ('_' != pc[3] || 'u' != pc[0])
			break;
	}
	pQ->ijobs = n;
	pQ->ppDElist = ppDEList;
}

/* free the memory bought when we scanQ'd				(ksb)
 */
static void
FreeQ(pQ)
Q *pQ;
{
	register int i;

	if ((struct DIRENT **)0 != pQ->ppDErun) {
		pQ->ijobs += pQ->irun + pQ->iuser;
		for (i = 0; i < pQ->ijobs; ++i) {
			if ((struct DIRENT *)0 != pQ->ppDErun[i]) {
				free((char *)pQ->ppDErun[i]);
			}
		}
		free((char *)pQ->ppDErun);
		pQ->ppDErun = pQ->ppDElist = (struct DIRENT **)0;
	}
	pQ->irun = pQ->iuser = pQ->ijobs = 0;
}


/* form the next name of a queue file in a queue at a priority and key	(ksb)
 * like `k01+03.001'
 *       0123456789
 */
static void
QName(pQ, pcOut, cKey, iPri)
register Q *pQ;
char *pcOut, cKey;
int iPri;
{
	register int i;
	register int iSeq;
	auto short int aiUniq[256];

	/* find the largest id used under each letter
	 */
	for (i = 0; i < 256; ++i) {
		aiUniq[i] = 1;
	}

	iSeq = 1;
	for (i = 0; i < pQ->ijobs; ++i) {
		register int t;
		register char *pcQFile;

		pcQFile = pQ->ppDElist[i]->d_name;
		t = atoi(pcQFile+1);
		if (t >= aiUniq[pcQFile[0]])
			aiUniq[pcQFile[0]] = t+1;
		if (atoi(pcQFile+3) == iPri)
			iSeq = atoi(pcQFile+7)+1;
	}

	/* find our new (bigger) id
	 * use the first letter of our login if those are not all taken
	 */
	if (isupper(cKey))
		cKey = tolower(cKey);
	while (aiUniq[cKey] > 100) {
		if (++cKey > 'z')
			cKey = 'a';
	}

	/* OK we are now this queue file (which encodes stuff we know)
	 */
	sprintf(pcOut, "%c%02d%+03d.%03d", cKey, aiUniq[cKey], iPri, iSeq);
}

#if 0
/* form the name of a running job given the pid				(ksb)
 */
static void
RName(pQ, pcOut, iPid)
register Q *pQ;
char *pcOut;
int iPid;
{
	(void)sprintf(pcOut, acProtoRunFmt, iPid);
}
#endif
