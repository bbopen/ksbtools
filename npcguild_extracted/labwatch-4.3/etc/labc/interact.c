/* $Id: interact.c,v 2.0 1994/05/22 16:41:08 ksb Two $
 * everything (almost) we need to do interactive commands for "labc".
 * Kevin Braundorf (ksb)
 */
#include <stdio.h>
#include <sys/param.h>
#include <ctype.h>

#include "machine.h"
#include "main.h"
#include "interact.h"
#include "stagger.h"

/* used by cmVersion! -- ksb */
static char rcsid[] =
	"$Id: interact.c,v 2.0 1994/05/22 16:41:08 ksb Two $";

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#define CMDLINELEN	256		/* maximum length user can type	*/

typedef struct CMnode {
	char *pccmd;
	char *pcdescription;
	int (*pficall)();
} CMD;

#if 0
/* for commands we have not implemented (yet)				(ksb)
 */
static int
cmZZZ(argc, argv)
int argc;
char **argv;
{
	printf("%s: %s: command not implemented (yet)\n", progname, argv[0]);
	return 1;
}
#endif

/* the classic ksb help command						(ksb)
 */
static int
cmHelp(argc, argv)
int argc;
char **argv;
{
	extern CMD aCMLabd[];
	register int i, iWidth;
	register char *pc;

	iWidth = 4;
	for (i = 0; (char *)0 != (pc = aCMLabd[i].pccmd); ++i) {
		register int l;
		l = strlen(pc)+2;
		if (iWidth < l)
			iWidth = l;
	}
	--iWidth;
	for (i = 0; (char *)0 != (pc = aCMLabd[i].pccmd); ++i) {
		register char *pcDescr;
		if ((char *)0 == (pcDescr = aCMLabd[i].pcdescription))
			continue;
		printf("%-*s %s\n", iWidth, pc, pcDescr);
	}
	return 0;
}

/* the command to get you out cleanly					(ksb)
 */
static int /*ARGSUSED*/
cmExit(argc, argv)
int argc;
char **argv;
{
	exit(0);
}

/* the classic ksb version command					(ksb)
 */
static int
cmVersion(argc, argv)
int argc;
char **argv;
{
	printf("%s: %s\n", progname, rcsid);
	return 0;
}

char acTickets[32];

/* disable tickets for labd -> labwatch					(ksb)
 */
static int
cmDisable(argc, argv)
int argc;
char **argv;
{
	return Tell("N", acTickets, 1);
}

/* enable tickets for labd -> labwatch					(ksb)
 */
static int
cmEnable(argc, argv)
int argc;
char **argv;
{
	return Tell("Y", acTickets, 1);
}

/* output a packet from labd						(ksb)
 */
static int
cmPing(argc, argv)
int argc;
char **argv;
{
	auto char acStats[MAXUDPSIZE];

	acStats[0] = '\000';
	(void)Tell("!", acStats, 1);
	if ('\000' == acStats[0]) {
		return 1;
	}
	acTickets[0] = acStats[0];
	acTickets[1] = '\000';
	/* LLL format the packet we got back?
	 */
	printf("%s", acStats);
}

/* kill the labd on the target machines					(ksb)
 */
static int
cmShoot(argc, argv)
int argc;
char **argv;
{
	return Tell("q", (char *)0, 0);
}

CMD aCMLabd[] = {
	{"bye",		(char *)0, cmExit},
	{"control",	"change which host(s) we control", cmControl},
	{"disable",	"do not grant tickets for this machine", cmDisable},
	{"enable",	"grant tickets for this machine", cmEnable},
	{"help", 	"print help information", cmHelp},
	{"message",	"send a message to labwatch from this host", cmMessage},
	{"ping",	"poll labd for current info", cmPing},
	{"quit",	"exit this program", cmExit},
	{"redirect",	"move labd to another server", cmRedir},
	{"shoot",	"terminate labd on this host", cmShoot},
	{"shutdown",	"let labwatch know we are going down soon", cmReboot},
	{"stagger",	"spread out clients over the labd report interval", cmStagger},
	{"status",	"output useful status information", cmStatus},
	{"version", 	"display the version of labc", cmVersion},
	{"?", 		(char *)0, cmHelp},
	{(char *)0, (char *)0, (int (*)())0}
};

/* this trys to emulate shell quoting, but I doubt it does a good job	(ksb)
 * [[ but not substitution -- that would be silly ]]
 */
static char *
mynext(pcScan, pcDest)
register char *pcScan, *pcDest;
{
	register int fQuote;

	for (fQuote = 0; *pcScan != '\000' && (fQuote||(*pcScan != ' ' && *pcScan != '\t')); ++pcScan) {
		switch (fQuote) {
		default:
		case 0:
			if ('"' == *pcScan) {
				fQuote = 1;
				continue;
			} else if ('\'' == *pcScan) {
				fQuote = 2;
				continue;
			}
			break;
		case 1:
			if ('"' == *pcScan) {
				fQuote = 0;
				continue;
			}
			break;
		case 2:
			if ('\'' == *pcScan) {
				fQuote = 0;
				continue;
			}
			break;
		}
		if ((char*)0 != pcDest) {
			*pcDest++ = *pcScan;
		}
	}
	if ((char*)0 != pcDest) {
		*pcDest = '\000';
	}
	return pcScan;
}

/* given an envirionment variable insert it in the option list		(ksb)
 * (exploded with the above routine)
 */
static int
breakargs(cmd, pargc, pargv)
char *cmd, *(**pargv);
int *pargc;
{
	register char *p;		/* tmp				*/
	register char **v;		/* vector of commands returned	*/
	register unsigned sum;		/* bytes for malloc		*/
	register int i, j;		/* number of args		*/
	register char *s;		/* save old position		*/

	while (*cmd == ' ' || *cmd == '\t')
		cmd++;
	p = cmd;			/* no leading spaces		*/
	i = 1 + *pargc;
	sum = sizeof(char *) * i;
	while (*p != '\000') {		/* space for argv[];		*/
		++i;
		s = p;
		p = mynext(p, (char *)0);
		sum += sizeof(char *) + 1 + (unsigned)(p - s);
		while (*p == ' ' || *p == '\t')
			p++;
	}
	++i;
	/* vector starts at v, copy of string follows NULL pointer
         * the extra 7 bytes on the end allow use to be alligned
         */
        v = (char **)malloc(sum+sizeof(char *)+7);
	if (v == NULL)
		return 0;
	p = (char *)v + i * sizeof(char *); /* after NULL pointer */
	i = 0;				/* word count, vector index */
	v[i++] = (*pargv)[0];
	while (*cmd != '\000') {
		v[i++] = p;
		cmd = mynext(cmd, p);
		p += strlen(p)+1;
		while (*cmd == ' ' || *cmd == '\t')
			++cmd;
	}
	for (j = 1; j < *pargc; ++j)
		v[i++] = (*pargv)[j];
	v[i] = NULL;
	*pargv = v;
	*pargc = i;
	return i;
}

/* interp the command and run it					(ksb)
 * break into an argv, find in table, call, cleanup argv, pass back `exit'
 */
int
StrCmd(pcCmd)
char *pcCmd;
{
	register int i;
	register char *pc;
	auto int iArgc;
	auto char **ppcArgv;
	auto char *apc_[2];
	auto int iRet;

	iArgc = 0;
	apc_[0] = progname;
	apc_[1] = (char *)0;
	ppcArgv = apc_;
	breakargs(pcCmd, & iArgc, & ppcArgv);
	if (fDebug) {
		printf("%s: cmd:", progname);
		for (i = 1; i < iArgc; ++i) {
			printf(" `%s'", ppcArgv[i]);
		}
		printf("\n");
	}

	/* dispatch command */
	if ((char *)0 == ppcArgv[1]) {
		iRet = 0;
	} else {
		for (i = 0; (char *)0 != (pc = aCMLabd[i].pccmd); ++i) {
			if (0 == strcmp(pc, ppcArgv[1])) {
				break;
			}
		}
		if ((char *)0 != pc) {
			iRet = (aCMLabd[i].pficall)(iArgc-1, ppcArgv+1);
		} else {
			iRet = 1;
			fprintf(stderr, "%s: %s: unknown command (try \"help\")\n", progname, ppcArgv[1]);
		}
	}

	free((char *)ppcArgv);

	return iRet;
}

/* grab a line from stdin and give it to the command driver		(ksb)
 * we also prompt cleverly
 */
int
Interactive()
{
	static char acCmdBuf[CMDLINELEN];
	register char *pc;
	register FILE *fpPrompt;
	auto int iRet;

	iRet = 0;
	if (!isatty(0))
		fpPrompt = (FILE *)0;
	else if (isatty(1))
		fpPrompt = stdout;
	else if (isatty(2))
		fpPrompt = stderr;
	else
		fpPrompt = (FILE *)0;
		
	for (;;) {
		if ((FILE *)0 != fpPrompt) {
			(void)fprintf(fpPrompt, "%s ", progname);
			(void)fflush(fpPrompt);
		}

		if (NULL == fgets(acCmdBuf, CMDLINELEN, stdin)) {
			if ((FILE *)0 != fpPrompt) {
				(void)fprintf(fpPrompt, "[EOF]\n");
			}
			exit(0);
		}
		if (NULL != (pc = strchr(acCmdBuf, '\n'))) {
			*pc = '\000';
		}
		if ('#' == acCmdBuf[0]) {
			continue;
		}
		iRet |= StrCmd(acCmdBuf);
	}

	return iRet;
}
