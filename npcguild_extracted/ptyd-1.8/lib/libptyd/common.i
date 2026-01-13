extern int strncmp();
extern char *strcpy(), *strchr(), *strcpy(), *strcat();
extern char *malloc(), *realloc();

/*
 * reply to a massage from the user, build a buffer and sprinf into it	(ksb)
 */
static int
Reply(sMsg, pcCmd, pcArg)
int sMsg;
char *pcCmd, *pcArg;
{
	register int iLen;
	static int iHave;
	static char *pcBuf = (char *)0;


	/* total length of string including the "\n\00" on the end
	 */
	       /* "take"      " "  "/dev/ptyp5" "\n\000" */
	iLen = strlen(pcCmd) + 1 + strlen(pcArg) + 2;

	/* get a buffer for the string
	 */
	if ((char *)0 == pcBuf) {
		pcBuf = malloc(iLen+8);
		iHave = iLen;
	} else if (iLen > iHave) {
		pcBuf = realloc(pcBuf, iLen+8);
		iHave = iLen;
	}
	if ((char *)0 == pcBuf) {
		write(sMsg, "server out of memory\n", 20);
		exit(1);
	}

	/* fill the buffer and send it
	 */
	(void) strcat(strcat(strcpy(pcBuf, pcCmd), " "), pcArg);
	(void) strcat(pcBuf, "\n");
	if (iLen-1 != write(sMsg, pcBuf, iLen-1)) {
		return -1;
	}

	return 0;
}

/*
 * be sure every line sent has a new line on the end, follow protocol	(ksb)
 * (end of file is cool as is)
 */
static int
Read(fd, pc, cnt)
int fd, cnt;
char *pc;
{
	register int r;

	if (0 > (r = read(fd, pc, cnt))) {
		return -1;
	}
	if (0 == r) {
		return 0;
	}
	if ('\n' != pc[--r]) {
		return -1;
	}
	pc[r] = '\000';
	return r;
}

/*
 * return the tty that goes with this pty				(ksb)
 */
static char *
WhichTty(pcTty, pcPty)
char *pcTty, *pcPty;
{
	register char *pc;

	pc = strcpy(pcTty, pcPty);
	while ((char *)0 != (pc = strchr(pc, 'p'))) {
		if (0 == strncmp(pc, "pty", 3)) {
			break;
		}
		++pc;
	}
	if ((char *)0 == pc || 'p' != *pc) {
		return (char *)0;
	}

	*pc = 't';
	return pcTty;
}

/*
 * look to see if we were given a command we recognize,			(ksb)
 * return the next argument
 */
static char *
IsCmd(pcParse, pcCmd)
char *pcParse, *pcCmd;
{
	register int iLen;

	while (isspace(*pcParse))
		++pcParse;
	while (isspace(*pcCmd))
		++pcCmd;
	if (0 != strncmp(pcCmd, pcParse, iLen = strlen(pcCmd))) {
		return (char *)0;
	}
	pcParse += iLen;
	while (isspace(*pcParse))
		++pcParse;
	return pcParse;
}

/*
 * Below is the string for finding /dev/ptyXX.  For each architecture we
 * leave some pty's world writable because we don't have source for 
 * everything that uses pty's.  For the most part, we'll be trying to
 * make /dev/ptyq* the "free" pty's.
 */
#if SUN3 || SUN4
static char charone[] =
	"prstuvwxyzPQRSTUVWq";
#else

#if S81
static char charone[] =
	"prstuvwxyzPQRSTUVWq";
#else

#if VAX8800
static char charone[] =
	"prstuvwxyzPQRSTUVWq";
#else

/* all the world's a vax ;-) */
static char charone[] =
	"prstuvwxyzPQRSTUVWq";
#endif
#endif
#endif

static char chartwo[] =
	"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
#if IBMR2 || OLDR2
static char acMaster[] =
	"/dev/ptc/XXXXXXXXX";
static char acSlave[] =
	"/dev/pts/XXXXXXXXX";
#else
static char acMaster[] =
	"/dev/ptyXX";
static char acSlave[] =
	"/dev/ttyXX";
#endif /* _AIX */

#if !HAVE_GETPSEUDO
#if IBMR2 || OLDR2
/*
 * get a pty for the user (emulate the neato sequent call)		(ksb)
 */
static int
getpseudotty(slave, master)
char **master, **slave;
{
	int fd;
	char *pcName, *pcTmp;

	if (0 > (fd = open("/dev/ptc", O_RDWR|O_NDELAY, 0))) {
		return -1;
	}
	if ((char *)0 == (pcName = ttyname(fd))) {
		return -1;
	}
	strcpy(acSlave, pcName);
	*slave = acSlave;

	strcpy(acMaster, pcName);
	acMaster[7] = 'c';
	*master = acMaster;

	return fd;
}

#else
/*
 * get a pty for the user (emulate the neato sequent call)		(ksb)
 */
static int
getpseudotty(slave, master)
char **master, **slave;
{
	static char *pcOne = charone, *pcTwo = chartwo;
	auto int fd, iLoop, iIndex = sizeof("/dev/pty")-1;
	auto char *pcOld1;
	auto struct stat statBuf;

	iLoop = 0;
	pcOld1 = pcOne;
	for (;;) {
		if ('\000' == *++pcTwo) {
			pcTwo = chartwo;
			if ('\000' == *++pcOne) {
				pcOne = charone;
				if (pcOld1 == pcOne && ++iLoop > 1 || iLoop > 32)
					return -1;
			}
		}
		acMaster[iIndex] = *pcOne;
		acMaster[iIndex+1] = *pcTwo;

		/*
		 * Remeber we are root - stat the file
		 * to see if it exists before we open it
		 * for read/write - if it doesn't we don't
		 * have any pty's left in the row
		 */
		if (-1 == stat(acMaster, &statBuf) || S_IFCHR != (statBuf.st_mode&S_IFMT)) {
			pcTwo = "l";
			continue;
		}

		if (0 > (fd = open(acMaster, O_RDWR|O_NDELAY, 0))) {
			continue;
		}
		acSlave[iIndex] = *pcOne;
		acSlave[iIndex+1] = *pcTwo;
		if (-1 == access(acSlave, F_OK)) {
			(void) close(fd);
			continue;
		}
		break;
	}
	*master = acMaster;
	*slave = acSlave;
	return fd;
}
#endif /* _AIX */
#endif /* !HAVE_GETPSEUDO */
