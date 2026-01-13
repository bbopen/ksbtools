#!mkcmd
#* +--------------------------------------------------------------------+ */
#* | Copyright 1991, David Koblas.					| */
#* |   Permission to use, copy, modify, and distribute this software	| */
#* |   and its documentation for any purpose and without fee is hereby	| */
#* |   granted, provided that the above copyright notice appear in all	| */
#* |   copies and that both that copyright notice and this permission	| */
#* |   notice appear in supporting documentation.  This software is	| */
#* |   provided "as is" without express or implied warranty.		| */
#* +--------------------------------------------------------------------+ */
# This version has been modified substantially by KS Braunsdorf, for
# example it is coded in mkcmd rather than pure C, and it know about lots
# more mnemonic attributes, and -u, -g, -f options.
from '<stdio.h>'
from '<syslog.h>'
from '<pwd.h>'
from '<errno.h>'
from '<grp.h>'
from '<ctype.h>'
from '<sys/time.h>'
from '<sys/types.h>'
from '<sys/wait.h>'
from '<sys/socket.h>'
from '<sys/un.h>'
from '<sys/resource.h>'
from '<sys/param.h>'
from '<sys/stat.h>'
from '<sysexits.h>'
from '<unistd.h>'
from '<limits.h>'
from '<stdlib.h>'
from '<fcntl.h>'
from '<dirent.h>'
from '<string.h>'
from '<netdb.h>'
from '<regex.h>'

from '"machine.h"'
from '"main.h"'

require "util_getlogingr.m" "util_cache.m"
require "std_help.m" "std_version.m"

%i
static const char acDefPam[] =
	"op";

#if USE_PAM
#include <security/pam_appl.h>
#endif
%%
named "Progname"

init 3 'openlog(acDefPam, 0, LOG_AUTH);'
init 4 'CheckIndirect(%#, %@);'
after 'if (fReadExisting) ReadFile(acAccess);'

char* 'u' {
	named "pcUid" track "fUid"
	init "(char *)0"
	user "CheckColon(& %n, & %(respect)gn);"
	param "login"
	help "specify the login for the process, as allowed"
}
char* 'g' {
	named "pcGid" track "fGid"
	init "(char *)0"
	param "group"
	help "specify the group for the process, as allowed"
}
char* 'f' {
	named "pcSecFile" track "fSecFile"
	init "(char *)0"
	param "file"
	help "specify direct file for the $f macro, as allowed"
}

boolean 'S' {
	exclude "fguHl"
	named "fSanity"
	help "sanity check rule database"
	boolean 'n' {
		named "fReadExisting"
		init '1'
		help "do not read the existing rule-base"
	}
	list {
		named "SanityCheck"
		param "files"
		help "optional configuration files to check"
	}
}
int named "fWhichList" {
	init "'_'"
	local help ""
}
boolean 'lrw' {
	exclude "fguHS"
	help "list rules available to this session (list, run, why)"
	update "fWhichList = %w;"
	list {
		named "RulesAllowed"
		update "%n(fWhichList, %#, %@);"
		param "login"
		help "the superuser may request a list for any login"
	}
}

augment action 'V' {
	user "Version();"
}

action 'H' {
	update "ConfHelp(stdout);"
	aborts "exit(EX_OK);"
	help "show only help on the configuration of this program"
}

list {
	param "mnemonic"
	named "OldMain"
	update "%n(%#, (const char **)%@);"
	help "specify the nemonic and any arguments"
}

zero {
	named ""
	update "/* none */"
	aborts 'fatal("provide mnemonic, or use -h for help");'
}

%h
/* Each configuration stanza is represented by an `op definition' structure.
 */
typedef struct ODnode {
	const char	*pcname;	/* mnemonic name		*/
	int		nargs, nopts;	/* current counts		*/
	int		margs, mopts;	/* max allocated currently	*/
	int		iline;		/* defining line and file	*/
	const char	*pcfile;
	const char	**args, **opts;	/* vectors of args and attrs	*/
	struct ODnode	*pODnext;	/* next command in order	*/
	struct ODnode	*pODdefscope;	/* the last DEFAULT we saw	*/
	int		fcarp;		/* flag use in sanity checks	*/
	const char	*pcscript;	/* { inline script } text	*/
} cmd_t;
%%

%i
static char rcsid[] =
	"$Id: op.m,v 2.81 2010/07/27 22:08:01 ksb Exp $";

#if !defined(HAVE_SETFIB)
#define HAVE_SETFIB	defined(SO_SETFIB)
#endif

#if HAVE_SETFIB
#include <sys/sysctl.h>
#endif

#if VARARGS
#include <varargs.h>
#else /* VARARGS */
#include <stdarg.h>
#endif /* VARARGS */

#if USE_GETSPNAM
#include <shadow.h>
#endif

#if !defined(OP_GIDLIST_MAX)
#if defined(NGROUPS)
#define OP_GIDLIST_MAX	NGROUPS
#else
#if defined(NGROUPS_MAX)
#define OP_GIDLIST_MAX	NGROUPS_MAX
#else
#define OP_GIDLIST_MAX	256
#endif
#endif
#endif

#if !defined(OP_MAX_LDWIDTH)
#define OP_MAX_LDWIDTH	32	/* length of a printf("_%ld", max_long) */
#endif

#if !defined(PATH_SUDO)
#define PATH_SUDO	"sudo"
#endif

static const char
	*pcPerp = "<notme>",
	*pcPerpHome = (const char *)0,
	*pcCmd = (const char *)0,
	acAccess[] = ACCESS_FILE,
	acDefault[] = "DEFAULT",
	acDefShell[] = "/bin/sh",
	acDevTty[] = "/dev/tty",
	acMagicShell[] = "MAGIC_SHELL",
	acOptDaemon[] = "daemon",
	acOptNetgroups[] = "netgroups",		/* the three cred types */
	acOptGroups[] = "groups",
	acOptUsers[] = "users",
	acOptPass[] = "password",		/* must type a password */
	acOptPam[] = "pam",			/* like password, with pam */
	acOptSession[] = "session",		/* let pam build a session */
	acOptCleanup[] = "cleanup",		/* close the session */
	acOK[] = "Access granted",		/* little information */
	acSure[] = ", are you sure?",		/* grep -v this when you are */
	acBadAllow[] = "Permission denied",
	acBadConfig[] = "Configuration error",	/* help the Admin */
	acBadRE_s[] = "%s: RE compilation error",
	acBadAlloc[] = "Out of memory",		/* out of resources */
	acBadList[] = "No matching parameter list", /* radiate some info */
	acBadParam[] = "Argument rejected",
	acBadNumber[] = "Wrong number of arguments",
	acBadFew[] = "Too few arguments",
	acBadMnemonic[] = "No such mnemonic";

static cmd_t *NewCmd(const char *pcName);

#if defined(DEBUG)
/* Dump out the given command about the way we read it.			(ksb)
 */
static const cmd_t *
CmdOutput(const cmd_t *cmd)
{
	register int i;

	fprintf(stderr, "%s\t", cmd->pcname);
	for (i = 0; i < cmd->nargs; ++i)
		fprintf(stderr, "%s ", cmd->args[i]);
	if ((cmd_t *)0 != cmd->pODdefscope) {
		fprintf(stderr, "[defaults from %s:%d] ", cmd->pODdefscope->pcfile, cmd->pODdefscope->iline);
	}
	fprintf(stderr, ";\n");
	for (i = 0; i < cmd->nopts; ++i)
		fprintf(stderr, "\t%s\n", cmd->opts[i]);
	fprintf(stderr, "\n");
	return cmd;
}
#else
#define CmdOutput(Mcmd)	(Mcmd)
#endif /* DEBUG */

/* Varargs function to output a message to stderr and exit failure.	(orig)
 * Also syslogs the access failure when pcCmd is set.
 */
#if VARARGS
static void
fatal(va_alist)
va_dcl
{
	auto va_list	ap;
	register char *s;

	va_start(ap);
	if ((const char *)0 != pcCmd) {
		syslog(LOG_NOTICE, "user %s FAILED to execute %s", pcPerp, pcCmd);
	}
	s = va_arg(ap, char *);
	fprintf(stderr, "%s: ", Progname);
	vfprintf(stderr, s, ap);
	fputc('\n', stderr);
	va_end(ap);

	exit(EX_DATAERR);
	/*NOTREACHED*/
}
#else /* VARARGS */
static void
fatal(const char *format, ...)
{
	auto va_list ap;

	va_start(ap, format);
	if ((const char *)0 != pcCmd) {
		syslog(LOG_NOTICE, "user %s FAILED to execute %s", pcPerp, pcCmd);
	}
	fprintf(stderr, "%s: ", Progname);
	vfprintf(stderr, format, ap);
	fputc('\n', stderr);
	va_end(ap);

	exit(EX_DATAERR);
	/*NOTREACHED*/
}
#endif /* VARARGS */

/* If op is installed non-setuid, indirect through sudo to get root	(ksb)
 * access, that way we can run on a host that doesn't let us install setuid
 * but does allow a policy to delegate details to op.  How strange.
 * Naming a mnemonic "op" might confuse the loop check below.
 *
 * In sudoers allow "op" to run us with any args, euid as root, uid as
 * themselves, I'd leave the groups as-is.  I don't use this feature.
 */
static void
CheckIndirect(int argc, char **argv)
{
	register int i;
	register char **ppcNew;

	if (0 == geteuid() || 0 == getuid())
		return;

	if ((char *)0 != argv[1] && 0 == strcmp(Progname, argv[1]))
		fprintf(stderr, "%s: possible sudo/op loop\n", Progname);

	/* Building: "sudo" Progname argv[0] argv[1]... (char *)0
	 */
	if ((char **)0 == (ppcNew = calloc((argc|3)+5, sizeof(char *))))
		fatal(acBadAlloc);
	i = 0;
	ppcNew[i++] = PATH_SUDO;
	for (ppcNew[i++] = Progname; 0 != argc--; /* move along */)
		ppcNew[i++] = *argv++;
	ppcNew[i] = (char *)0;
	(void)execvp(*ppcNew, ppcNew);
	fatal("execvp: %s: %s", *ppcNew, strerror(errno));
	/*NOTREACHED*/
}

/* Keep track of a gid list, only add the unique ones.			(ksb)
 */
static void
AddGid(const gid_t gNew, int *pnCount, gid_t *pwSet)
{
	register int i;

	for (i = 0; i < *pnCount; ++i) {
		if (*pwSet == gNew)
			return;
		++pwSet;
	}
	if (OP_GIDLIST_MAX == *pnCount) {
		fatal("gid: out of space in grouplist");
	}
	*pwSet = gNew;
	++*pnCount;
}

/* Search for keywords, like "uid" or "uid=pat"				(orig)
 */
static char *
FindOpt(const cmd_t *cmd, const char *pcAttr)
{
	register int i;
	register char *cp;
	register int iLook;
	static char acEmpty[2] = "";

	iLook = strlen(pcAttr);
	for (i = 0; i < cmd->nopts; ++i) {
		if ((char *)0 == (cp = strchr(cmd->opts[i], '='))) {
			/* uid w/o an = is the same as the empty string */
			if (0 == strcmp(cmd->opts[i], pcAttr))
				return acEmpty;
		} else if (iLook != (cp - cmd->opts[i])) {
			continue;
		} else if (0 == strncmp(cmd->opts[i], pcAttr, iLook)) {
			return cp+1;
		}
	}
	return (char *)0;
}

/* Copy out each of N comma separated values in order, a comma before	(ksb)
 * a comma removes the separator meaning -- better than a backslash.  Empty
 * fields don't help op any.
 * Call with pcCur as NULL to reset the memory buffer.
 */
static const char *
GetField(const char *pcCur, char **ppcProcess)
{
	register char *pcOut;
	register size_t iLen;
	register int c;
	static char *pcKeep = (char *)0;
	static size_t iKeep = 0;

	if ((char **)0 == ppcProcess) {
		if ((char *)0 != pcKeep) {
			free((void *)pcKeep);
			pcKeep = (char *)0;
			iKeep = 0;
		}
	}
	if ('\000' == *pcCur) {
		*ppcProcess = (char *)0;
		return (char *)0;
	}
	if (((iLen = strlen(pcCur)) > iKeep) || (char *)0 == pcKeep) {
		/* Since we use the same buffer for all calls make
		 * it big enough we might not need to resize it.
		  */
		iKeep = (iLen|4095)+1;
		if ((char *)0 == pcKeep)
			pcKeep = malloc(iKeep);
		else
			pcKeep = realloc((void *)pcKeep, iKeep);
	}
	if ((char *)0 == pcKeep) {
		fatal(acBadAlloc);
	}
	if ((char **)0 != ppcProcess) {
		*ppcProcess = pcKeep;
	}
	for (pcOut = pcKeep; '\000' != (c = *pcCur); ++pcCur) {
		if (',' == c && ',' != pcCur[1])
			break;
		*pcOut++ = c;
		if (',' == c)
			++pcCur;
	}
	*pcOut = '\000';

	return (*pcCur == '\000') ? pcCur : (pcCur+1);
}

static const char *pcLastTag = acBadMnemonic;

/* Since we only ever have on RE compiled at a time, and we want to	(ksb)
 * diddle the RE for some tags we focuse all the RE compiles here.
 * When (const char *)0 == pcTag, let the called fatal for us.
 */
static regex_t *
ReComp(const char *pcRE, const char *pcTag)
{
	static int fAlt = 0;
	static regex_t aRE[2];
	register int iErr, iLen;
	register const char *pcEq;
	auto char acMesg[1024];

	if ((const char *)0 == pcRE) {
		return & aRE[fAlt];
	}
	fAlt ^= 1;
	pcLastTag = pcTag;
	if (0 == (iErr = regcomp(& aRE[fAlt], pcRE, REG_EXTENDED|REG_NOSUB))) {
		return & aRE[fAlt];
	}
	(void)regerror(iErr, &aRE[fAlt], acMesg, sizeof(acMesg));
	if ((const char *)0 == pcTag) {
		fprintf(stderr, "%s: regcomp: %s\n", Progname, acMesg);
		return (regex_t *)0;
	}
	if ((char *)0 == (pcEq = strchr(pcTag, '='))) {
		iLen = strlen(pcTag);
	} else {
		iLen = pcEq-pcTag;
	}
	fatal("%.*s: regcomp: %s", iLen, pcTag, acMesg);
	/*NOTREACHED*/
	return 0;
}

/* Match against the last compiled RE.					(ksb)
 */
static int
ReMatch(regex_t *pRE, const char *pcValue)
{
	register int iErr;
	auto regmatch_t aRM[1];
	auto char acMesg[1024];

	if (pRE != ReComp((const char *)0, (const char *)0)) {
		fatal("please report an internal RE sync bug!");
	}
	aRM[0].rm_so = 0;
	aRM[0].rm_eo = strlen(pcValue);
	switch (iErr = regexec(pRE, pcValue, 1, aRM, 0)) {
	case 0:
		return 1;
	case REG_NOMATCH:
		return 0;
	default:
		break;
	}
	(void)regerror(iErr, pRE, acMesg, sizeof(acMesg));
	fatal("%s: regexec: %s", pcLastTag, acMesg);
	/*NOTREACHED*/
	return 0;
}

/* Return 1 when any field from any listed attribute contains token.	(ksb)
 * List is a concatenation of strings terminated by a double-\000 (constr).
 */
static int
AnyField(const cmd_t *cmd, const char *pcToken, const char *psList)
{
	register const char *pcLook;
	register regex_t *reg;
	register int iRet;
	auto char *pcField;

	if ((regex_t *)0 == (reg = ReComp(pcToken, (const char *)0))) {
		fatal(acBadRE_s, pcToken);
	}
	iRet = 0;
	for (/* param */; 0 == iRet && '\000' != *psList; psList += strlen(psList)+1) {
		if ((char *)0 == (pcLook = FindOpt(cmd, psList)))
			continue;
		while ((char *)0 != (pcLook = GetField(pcLook, &pcField)))
			if (0 != (iRet = ReMatch(reg, pcField)))
				break;
	}
	regfree(reg);
	return iRet;
}

/* Remember what a mnemonic `needs' on the command-line usage
 */
typedef struct NEnode {
	int	fUid,		/* saw %u, $u, $U			*/
		fGid,		/* saw %g, $g, $G			*/
		fSecFile,	/* saw %f, $f				*/
		fSecFd,		/* saw $F				*/
		fSecDir,	/* saw $d				*/
		fDirFd,		/* saw $D				*/
		fDStar;		/* saw $*, $@: makes argc variable	*/
	long	lCmdParam,	/* highest $[0-9]+ in command template	*/
		lHelpParam,	/* highest $[0-9]+ in spec for -l	*/
		lTotalParam;	/* $# count or lCmdParam or -1		*/
} NEEDS;

/* Which indirect objects (-g, -u, -f) we need from the command-line?
 * SecFd is only tripped by $F.  Look for the highest $n mentioned, and
 * limit argc to include that many words if no $* is mentioned.
 */
static int
Craving(const cmd_t *cmd, NEEDS *pNE)
{
	register int c, iArgx, iOptx, iRet;
	register const char *pcScan;
	auto char *pcEnd;
	auto unsigned long ul;

	pNE->fSecFile = AnyField(cmd, "^%f$", "uid\000euid\000gid\000egid\000initgroups\000session\000cleanup\000password\000dir\000chroot\000\000") || AnyField(cmd, "^[<>]*%f$", "stdin\000stdout\000stderr\000\000");
	pNE->fSecDir = AnyField(cmd, "^%d$", "dir\000chroot\000\000");
	pNE->fUid = AnyField(cmd, "^%u$", "uid\000euid\000gid\000egid\000initgroups\000session\000cleanup\000password\000\000") || AnyField(cmd, "^$|%u", "!g@u\000%g@u\000\000");
	pNE->fGid = AnyField(cmd, "^%g$", "gid\000egid\000\000");
	pNE->fSecFd = pNE->fDirFd = 0;
	pNE->fDStar = 0;
	pNE->lCmdParam = pNE->lHelpParam = 0;
	pNE->lTotalParam = -1;
	iRet = 0;
	for (iOptx = iArgx = 0; (iOptx < cmd->nopts && (char *)0 != (pcScan = cmd->opts[iOptx++])) || (iArgx < cmd->nargs && (char *)0 != (pcScan = cmd->args[iArgx++])); /* double cond */) {
		/* if looking for an environment variable
		 */
		if (0 == iArgx) {
			if (*pcScan++ != '$') {
				continue;
			}
			if ('@' == *pcScan || '*' == *pcScan) {
				continue;
			}
			if (isdigit(*pcScan)) {
				ul = strtol(pcScan, (char **)0, 10);
				if (-1 == pNE->lHelpParam || ul > (unsigned long)pNE->lHelpParam)
					pNE->lHelpParam = ul;
				continue;
			}
			if ('#' == *pcScan) {
				if ('=' != pcScan[1]) {
					fprintf(stderr, "%s: %s: %s:%d: $# has no default value\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline);
					iRet = 4;
					continue;
				}
				pNE->lTotalParam = strtol(pcScan+2, (char **)0, 10);
				if (0 > pNE->lTotalParam) {
					fprintf(stderr, "%s: %s: %s:%d: $# may not be negative (%s)\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcScan+2);
					pNE->lTotalParam = -1;
					iRet = 3;
					continue;
				}
				continue;
			}
		}
		while ('\000' != (c = *pcScan++)) {
			if ('$' != c) {
				continue;
			}
			if ('$' == (c = *pcScan++) || '_' == c || '|' == c || '@' == c || '*' == c || '#' == c || 'S' == c) {
				pNE->fDStar |= '@' == c || '*' == c;
				continue;
			}
			if ('s' == c) {
				if ((char *)0 == cmd->pcscript) {
					fprintf(stderr, "%s: %s: %s:%d: $s used without an in-line script\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline);
					iRet = 5;
				}
				continue;
			}
			if ((char *)0 != strchr("cChHiIlLrRaAtToOnNeE~bBwWpP", c)) {
				continue;
			}
			if ('{' == c) {
				if ((const char *)0 == (pcScan = strchr(pcScan, '}'))) {
					fprintf(stderr, "%s: %s: %s:%d: end of argument in ${env} expression\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline);
					return 1;
				}
				++pcScan;
				continue;
			}
			if ('\\' == c) {
				if ('\000' == *pcScan) {
					fprintf(stderr, "%s: %s: %s:%d: end of argument in backslash escape\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline);
					iRet = 2;
				} else {
					++pcScan;
				}
				continue;
			}
			if ('u' == c || 'U' == c) {
				pNE->fUid = 1;
				continue;
			}
			if ('g' == c || 'G' == c) {
				pNE->fGid = 1;
				continue;
			}
			if ('d' == c || 'D' == c) {
				pNE->fSecFile = 1;
				pNE->fSecDir = 1;
				pNE->fDirFd |= 'D' == c;
				continue;
			}
			if ('f' == c || 'F' == c) {
				pNE->fSecFile = 1;
				pNE->fSecFd |= 'F' == c;
				continue;
			}
			if (!isdigit(c)) {
				fatal("%s: unknown expansion $%c", cmd->pcname, c);
			}
			ul = strtoul(pcScan-1, & pcEnd, 10);
			pcScan = pcEnd;
			if (pNE->lCmdParam < ul) {
				pNE->lCmdParam = ul;
			}
		}
	}
	/* When no $* (or $@) is expanded set a fixed argument list.
	 */
	if (!pNE->fDStar && -1 == pNE->lTotalParam) {
		pNE->lTotalParam = pNE->lCmdParam;
	}
	if (-1 != pNE->lTotalParam && pNE->lHelpParam < pNE->lTotalParam) {
		pNE->lHelpParam = pNE->lTotalParam;
	}
	if (-1 != pNE->lTotalParam && pNE->lCmdParam > pNE->lTotalParam) {
		fprintf(stderr, "%s: %s: %s:%d: $%lu beyond given argument list\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pNE->lCmdParam);
		iRet = 1;
	}
	return iRet;
}

/* Generic attribute match, not a group or login, any(REs) ~= string.	(ksb)
 * Note that the OldSchool regexec we keep returns 1 for a match,
 * people used to the new interface be aware of that logic reversal.
 */
static int
GenMatch(const char *pcCause, const char *pcValue, const char *pcList)
{
	register const char *pcCur;
	register regex_t *reg;
	register int iRet;
	auto char *pcField;

	iRet = 0;
	if ((char *)0 == pcList) {
		return iRet;
	}
	for (pcCur = GetField(pcList, &pcField); 0 == iRet && (char *)0 != pcCur; pcCur = GetField(pcCur, &pcField)) {
		if ((regex_t *)0 == (reg = ReComp(pcField, pcCause)))
			fatal(acBadRE_s, pcField);
		iRet = ReMatch(reg, pcValue);
		regfree(reg);
	}
	return iRet;
}

/* Locate the command the Customer asked for from the list.		(ksb)
 * We match the $# first (if configured) then $1,$2,... here, but not $*
 *	rndc	/usr/sbin/rndc $1 ;
 *		$1=^start$,^stop$
 *	rndc	/etc/init.d/named $2 ;
 *		$1=^restart$
 */
static cmd_t *
Find(const char *pcMnemonic, const long iArgc, const char **ppcArgv, const char **ppcWrong)
{
	register int i, iCur;
	register cmd_t *cmd;
	register const char *pcCount, *pcAttr;
	auto char *pcEnd;
	auto char acCvt[OP_MAX_LDWIDTH];	/* $n from config	*/
	static char acDummy[] = "=.";		/* deault match for $n	*/

	for (cmd = NewCmd((const char *)0); (cmd_t *)0 != cmd; cmd = cmd->pODnext) {
		if (0 != strcmp(cmd->pcname, pcMnemonic)) {
			continue;
		}
		if ((char *)0 != (pcCount = FindOpt(cmd, "$#")) && iArgc != strtol(pcCount, &pcEnd, 0)+1) {
			if (acBadList != *ppcWrong)
				*ppcWrong = acBadNumber;
			continue;
		}
		for (i = 0; i < cmd->nopts; ++i) {
			pcAttr = cmd->opts[i];
			if (!('!' == pcAttr[0] || '$' == pcAttr[0]) || !isdigit(pcAttr[1])) {
				continue;
			}
			iCur = strtol(pcAttr+1, &pcEnd, 0);

			/* Trap !3 means argc <= 3,  vs  !3=. means "" != $3
			 * otherwise we fail when argc <= n (no $n)
			 */
			if ('!' == *pcAttr && '\000' == *pcEnd) {
				if (iArgc > iCur)
					break;
				continue;
			}
			if (iArgc <= iCur) {
				break;
			}
			snprintf(acCvt, sizeof(acCvt), "%.*s", pcEnd-pcAttr, pcAttr);
			if ('\000' == *pcEnd) {
				pcEnd = acDummy;
			}
			if (GenMatch(acCvt, ppcArgv[iCur], pcEnd+1)) {
				if ('!' == pcAttr[0])
					break;
			} else if ('$' == pcAttr[0]) {
				break;
			}
		}
		if (i == cmd->nopts)
			break;
		*ppcWrong = acBadList;
	}
	return cmd;
}
%%

%c
static char asFKnown[] =		/* see FileAttr below */
	"dev\000ino\000nlink\000atime\000mtime\000ctime\000btime\000birthtime\000size\000blksize\000blocks\000uid\000login@g\000login\000gid\000group\000mode\000path\000perms\000access\000type\000\000";
static char acMyself[] =
	".";

/* Info we need for version output.					(ksb)
 */
static void
Version()
{
	printf("%s: access file `%s'\n", Progname, acAccess);
	printf("%s: using regex\n", Progname);
	printf("%s: multiple configuration files accepted\n", Progname);
	printf("%s: inline script and $s accepted\n", Progname);
#if USE_PAM
	printf("%s: with pam support, default application \"%s\"\n", Progname, acDefPam);
#endif
}

static const char *apcConfHelp[] = {
	"$N\000the named positional parameter must match one of these REs",
	"$*\000every other positional parameter must match on of these REs",
	"$#\000count of the number of words allowed on the command line",
	"basename\000force a different argv[0] for the process",
	"chroot\000change root first, %f, %d, or a path",
#if USE_PAM
	"cleanup\000fork a process to close our PAM session, specificaion as session",
#else
	"cleanup\000[unsupported] fork a process to close our PAM session, specification as session",
#endif
	"daemon\000double-fork the process into the background and redirect I/O to /dev/null, boolean",
	"dir\000change directory here first, %f, %d, or a path",
	"environment\000allow all existing environment variables to pass, or limit to matching REs",
	"gid\000force the real uid to this group, number, %g, %f, or . (use the invoker's gid)",
	"egid\000make the effective gid different from the gid (same values)",
	"uid\000force the real uid to this login, number, %u, %f, or . (use the invoker's uid)",
	"euid\000make the effective uid different from the uid (same values)",
	"users\000allow execution only for logins that match one of these REs",
	"groups\000allow execution only for logins that have a group matching one of these REs",
	"netgroups\000allow execution for logins included in any listed group",
	"helmet\000a program to run a final check before execution",
	"jacket\000a clean-up program which is the parent of the new program",
	"initgroups\000set the supplementary groups based on uid, or the given login, %f or %u, or .",
	"nice\000bias the processes scheduling priority",
#if HAVE_SETFIB
	"fib\000specify an alternate network view",
#else
	"fib\000[unsupported platform] specify an alternate network view",
#endif
	"password\000ask for this login's password to credential the execution",
#if USE_PAM
	"pam\000pam application credentials required for execution, default as .",
	"session\000set session attributes for this login, %u, %f, or %i via pam",
#else
	"pam\000[unsupported] pam application credentials required for execution, default as .",
	"session\000[unsupported] set session attributes for this login, %u, %f, or %i via pam",
#endif
	"stdin\000set a forced standard input, %f or a path",
	"stdout\000set a forced standard output, %f or a path",
	"stderr\000set a forced standard error, %f or a path",
	"umask\000set the processes umask (default 022)",
	"$VAR\000pass the given environment variable on, optionally set a specific value",
	"%u\000-u's login must match one of these REs",
	"!u\000when -u's login matches any of these REs, reject the attempt",
	"%u@g\000allow a -u login that is in at least one group that matches one of these REs",
	"!u@g\000reject any -u login that is in any group that matches one of these REs",
	"%g\000-g's group must match one of these REs",
	"!g\000-g's group must not match any of these REs",
	"%g@u\000allow a -g group that contiains at least one login that matches one of these REs",
	"!g@u\000reject any -g group that contains any login that matches one of these REs",
	"%f.attr\000%f's stat(2) attr must match one of these REs",
	"!f.attr\000%f's stat(2) attr must not match any of these REs",
	"attr\000one of dev,ino,nlink,atime,mtime,ctime,btime,birthtime,size,blksize,blocks,uid,login,gid,group,login@g,mode,perms,path,access or type",
	(char *)0
};

/* Show the customer what op can take in a configuration file.		(ksb)
 */
static void
ConfHelp(FILE *fpOut)
{
	register int i, iWide, iLen;
	register const char *pc;

	fprintf(fpOut, "%s: configuration format:\nmnemonic\tcommand [args] ;\n\t\tkeyword[=value][,value...] ...\nkeywords:\n", Progname);
	iWide = 3;
	for (i = 0; (const char *)0 != (pc = apcConfHelp[i]); ++i) {
		iLen = strlen(pc);
		if (iLen > iWide)
			iWide = iLen;
	}
	for (i = 0; (char *)0 != (pc = apcConfHelp[i]); ++i) {
		fprintf(fpOut, "%*s %s\n", iWide, pc, pc+strlen(pc)+1);
	}
	fprintf(fpOut, "RE is an abbreviation for regular expression, see regex(3)\n");
	fflush(fpOut);
}

/* If -u ksb:sac then make it -u ksb and -g sac, like All Good Programs	(ksb)
 * _but_ should we set -g?  No.  That lets us suggest a group w/o forcing
 * the %g macro check.  Very subtle backward compatible, or a little broken.
 */
static void
CheckColon(char **ppcUser, char **ppcGroup)
{
	register char *pcColon;

	if ((char **)0 == ppcGroup || (char **)0 == ppcUser || (char *)0 == *ppcUser)
		return;
	if ((char *)0 == (pcColon = strchr(*ppcUser, ':')))
		return;
	*pcColon++ = '\000';
	*ppcGroup = pcColon;
}

static const char acDevNull[] = "/dev/null";

/* Return a new command structure on the end of the list.		(ksb)
 * When given a name of (char *)0 return the head command structure.
 */
static cmd_t *
NewCmd(const char *pcName)
{
	static cmd_t *pCMHead = (cmd_t *)0;
	static cmd_t **ppCMEnd = & pCMHead;
	register cmd_t *cmd;

	if ((char *)0 == pcName) {
		return pCMHead;
	}
	if ((cmd_t *)0 == (cmd = malloc(sizeof(cmd_t)))) {
		fatal(acBadAlloc);
	}
	cmd->pcname = pcName;
	cmd->fcarp = 0;
	cmd->nargs = cmd->nopts = 0;
	cmd->margs = cmd->mopts = 16;
	cmd->pcfile = acDevNull;
	cmd->iline = -1;
	cmd->args = (const char **)malloc(sizeof(char *)*cmd->margs);
	cmd->opts = (const char **)malloc(sizeof(char *)*cmd->mopts);
	cmd->pcscript = (const char *)0;
	if ((const char **)0 == cmd->args || (const char **)0 == cmd->opts) {
		fatal(acBadAlloc);
	}
	cmd->pODnext = (cmd_t *)0;
	*ppCMEnd = cmd;
	ppCMEnd = & cmd->pODnext;

	return cmd;
}

/* Simple list accumulation, not the way I'd do it.			(ksb)
 * We save memory here because we don't run all but one of these.  Must rules
 * have < 15 options or aguments.
 */
static void
AddElem(int *piMax, int *piCur, const char ***pppcVec, const char *pcValue)
{
	if (*piMax == *piCur) {
		*piMax += 32;
		*pppcVec = (const char **)realloc((void *)(*pppcVec), sizeof(char *) * *piMax);
		if ((const char **)0 == (*pppcVec))
			fatal(acBadAlloc);
	}
	(*pppcVec)[(*piCur)++] = pcValue;
}

/* Parse a whole file into command structure.				(ksb)
 * (Replaces the huge lex spec with the mkcmd file-cache code.)
 */
static void
OneFile(int fdIn, char *pcFile)
{
	register int c, cSawSemi;
	register unsigned int uCur, fEat;
	register cmd_t *cmd, *cmdDef;
	register char *pcCur, *pcTok, *pcDef;
	auto unsigned int uLines;

	cmd = cmdDef = (cmd_t *)0;
	pcDef = pcTok = (char *)0;
	cSawSemi = '\000';
	pcCur = cache_file(fdIn, &uLines);
	for (uCur = 1; uCur <= uLines; ++uCur, ++pcCur) {
		if ('\000' == *pcCur || '#' == *pcCur) {
			pcCur += strlen(pcCur);
			continue;
		}
		if (';' == *pcCur || '&' == *pcCur) {
			fatal("%s:%d \"%c\" is not a valid menomic", pcFile, uCur, *pcCur);
		}
		if (!isspace(*pcCur)) {
			if ((cmd_t *)0 != cmd && '&' == cSawSemi) {
				AddElem(&cmd->mopts, &cmd->nopts, &cmd->opts, acOptDaemon);
			}
			for (pcDef = pcCur; '\000' != (c = *pcCur); ++pcCur) {
				if (isspace(c) || ';' == c || '&' == c)
					break;
			}
			while (isspace(c)) {
				*pcCur++ = '\000';
				c = *pcCur;
			}
			/* The DEFAULT pseudo mnemonic is auto terminated
			 */
			cSawSemi = (0 == strcmp(pcDef, acDefault)) ? ';' : '\000';
			cmd = NewCmd('\000' != cSawSemi ? acDefault : pcDef);
			cmd->pcfile = pcFile;
			cmd->iline = uCur;
			cmd->pODdefscope = cmdDef;
			if ('\000' != cSawSemi) {
				cmdDef = cmd;
			}
			if (';' == c || '&' == c) {
				cSawSemi = c;
				*pcCur = '\000';
				c = *++pcCur;
			}

			/* mnemonic { # long shell script that
			 *	# spans mulitple lines and ends with a
			 *	# close curly  as the first non-white
			 *	} args ; ...
			 * run with $S -c $s $1 $2
			 * where $s is "the text w/o the outer curlies"
			 */
			if ('\000' == cSawSemi && '{' == c && ('\000' == pcCur[1] || isspace(pcCur[1]))) /*}*/{
				register char *pcEnd;

				cmd->pcscript = pcCur+1;
				while (uCur < uLines) {
					++uCur;
					pcCur += strlen(pcCur);
					*pcCur++ = '\n';
					while (isspace(*pcCur))
						++pcCur;
					if (/*{*/'}' == *pcCur)
						break;
				} /*{*/
				if ('}' != *pcCur) { /*{*/
					fatal("%s:%d: Missing '}' for in-line script", pcFile, uCur);
				}
				pcEnd = pcCur++;
				do {
					*pcEnd-- = '\000';
				} while ('\n' != *pcEnd);
				AddElem(&cmd->margs, &cmd->nargs, &cmd->args, "$S");
				AddElem(&cmd->margs, &cmd->nargs, &cmd->args, "-c");
				AddElem(&cmd->margs, &cmd->nargs, &cmd->args, "$s");
			}
		}

		for (fEat = 0; '\000' != (c = *pcCur); /* nope */) {
			if (fEat || isspace(c)) {
				++pcCur;
				continue;
			}
			if (';' == c || '&' == c) {
				++pcCur;
				if ((cmd_t *)0 == cmd) {
					fatal("%s:%d %c doesn't terminate a parameter list", pcFile, uCur, c);
				}
				if ('\000' != cSawSemi) {
					fatal("%s:%d double argument separator", pcFile, uCur);
				}
				if (0 == cmd->nargs && acDefault != cmd->pcname) {
					fatal("%s:%d %s: has no command template", pcFile, uCur, cmd->pcname);
				}
				cSawSemi = c;
				continue;
			}
			if ('#' == c) {
				++pcCur;
				fEat = 1;
				continue;
			}
			for (pcTok = pcCur; '\000' != (c = *pcCur); ++pcCur) {
				if (isspace(c))
					break;
			}
			if ('\000' != *pcCur) {
				*pcCur++ = '\000';
			}
			if ('\000' == *pcTok) {
				break;
			}
			if ((cmd_t *)0 == cmd) {
				fatal("%s:%d %s definitions must start in column 1", pcFile, uCur, pcTok);
			}
			if (fSanity && ('&' == pcCur[-1] || ';' == pcCur[-1])) {
				fprintf(stderr, "%s: -S: %s: %s:%d: \"%c\" on end of argument needs a space before it to begin attribute list\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcCur[-1]);
			}
			if ('\000' != cSawSemi) {
				AddElem(&cmd->mopts, &cmd->nopts, &cmd->opts, pcTok);
			} else {
				AddElem(&cmd->margs, &cmd->nargs, &cmd->args, pcTok);
			}
		}
	}
	if ((cmd_t *)0 != cmd && '\000' == cSawSemi) {
		fatal("%s:%d %s: missing definition", pcFile, uCur-1, cmd->pcname);
	}
	if ('&' == cSawSemi) {
		AddElem(&cmd->mopts, &cmd->nopts, &cmd->opts, acOptDaemon);
	}
}

/* Create the actual command spec from the default and the spec given.	(ksb)
 * If the first thing in access.cf is a DEFAULT rule then it applies
 * to all escalations w/o a DEAFAULT before their declatation.
 */
static cmd_t *
Build(const cmd_t *cmdOrig)
{
	register cmd_t *cmdRet, *cmdDef;
	register int i, iDef, iLen;
	register char *pcEq;

	cmdDef = NewCmd((const char *)0);
	if ((cmd_t *)0 != cmdDef && acDefault != cmdDef->pcname) {
		cmdDef = (cmd_t *)0;
	}
	if ((cmd_t *)0 != cmdOrig->pODdefscope) {
		cmdDef = cmdOrig->pODdefscope;
	}
	if ((cmd_t *)0 == cmdDef) {
		return (cmd_t *)cmdOrig;
	}
	if ((cmd_t *)0 == (cmdRet = malloc(sizeof(cmd_t)))) {
		fatal(acBadAlloc);
	}

	cmdRet->pcname = cmdOrig->pcname;
	cmdRet->iline = cmdOrig->iline;
	cmdRet->pcfile = cmdOrig->pcfile;
	cmdRet->nargs = cmdOrig->nargs;
	cmdRet->margs = cmdOrig->margs;
	cmdRet->mopts = ((cmdOrig->nopts+cmdDef->nopts)|3)+1;
	cmdRet->args = cmdOrig->args;
	cmdRet->opts = (const char **)malloc(sizeof(char *)*cmdRet->mopts);
	cmdRet->pODnext = (cmd_t *)0;
	cmdRet->pODdefscope = (cmd_t *)0;
	cmdRet->fcarp = cmdOrig->fcarp;
	cmdRet->nopts = cmdOrig->nopts;
	cmdRet->pcscript = cmdOrig->pcscript;
	for (i = 0; i < cmdOrig->nopts; ++i) {
		cmdRet->opts[i] = cmdOrig->opts[i];
	}

	for (iDef = 0; iDef < cmdDef->nopts; ++iDef) {
		if ((char *)0 == (pcEq = strchr(cmdDef->opts[iDef], '='))) {
			iLen = strlen(cmdDef->opts[iDef]);
		} else {
			iLen = pcEq - cmdDef->opts[iDef];
		}
		for (i = 0; i < cmdRet->nopts; ++i) {
			if (0 != strncmp(cmdRet->opts[i], cmdDef->opts[iDef], iLen)) {
				continue;
			}
			switch (cmdRet->opts[i][iLen]) {
			case '\000':
			case '=':
				break;
			default:
				continue;
			}
			break;
		}
		if (i == cmdRet->nopts) {
			cmdRet->opts[cmdRet->nopts++] = cmdDef->opts[iDef];
		}
	}
	cmdRet->opts[cmdRet->nopts] = (char *)0;
	return (cmd_t *)CmdOutput(cmdRet);
}

/* Check out a match of a group or login against a list of REs.		(ksb/pv)
 * Set pcTag to (const char *)0 and we'll ignore RE comp errors.
 */
static int
AnyMatch(const char *pcRegList, const char *pcName, const int iId, const char *pcTag)
{
	register const char *pcRE, *pcCheck;
	register int fMatch;
	register regex_t *reg;
	auto char acCvt[OP_MAX_LDWIDTH];
	auto char *pcField;

	snprintf(acCvt, sizeof(acCvt), "%d", iId);
	fMatch = 0;
	for (pcRE = GetField(pcRegList, &pcField); 0 == fMatch && (char *)0 != pcRE; pcRE = GetField(pcRE, &pcField)) {
		pcCheck = pcName;
		if ('#' == pcField[0]) {
			pcCheck = acCvt;
			if ((regex_t *)0 == (reg = ReComp(pcField+1, pcTag)))
				continue;
		} else if ((regex_t *)0 == (reg = ReComp(pcField, pcTag))) {
			continue;
		}
		if (ReMatch(reg, pcCheck))
			fMatch = 1;
		regfree(reg);
	}
	return fMatch;
}

/* Check the group file for membership of the login in any matching	(pv/ksb)
 * group.  Don't call getpw* here, the pcName is in the shared buf.
 */
static int
MemberMatch(const char *pcRegList, const char *pcName, const gid_t *pwGid, const char *pcTag)
{
	register struct group *grp;
	register int fMatch, i;
	register const char *pcRE;
	register regex_t *reg;
	auto char *pcField;

	fMatch = 0;
	setgrent();
	while (0 == fMatch && (struct group *)0 != (grp = getgrent())) {
		for (pcRE = GetField(pcRegList, &pcField); 0 == fMatch && (char *)0 != pcRE; pcRE = GetField(pcRE, &pcField)) {
			if ((regex_t *)0 == (reg = ReComp(pcField, pcTag)))
				continue;
			if (0 == ReMatch(reg, grp->gr_name)) {
				regfree(reg);
				continue;
			}
			regfree(reg);
			if ((gid_t *)0 != pwGid && *pwGid == grp->gr_gid) {
				fMatch = 1;
				break;
			}
			for (i = 0; (char *)0 != grp->gr_mem[i]; ++i) {
				if (0 == strcmp(pcName, grp->gr_mem[i])) {
					fMatch = 1;
					break;
				}
			}
		}
	}
	return fMatch;
}

/* Map a login name or #uid to a real uid, for %u.			(ksb)
 * Consult attributes %u, !u, and %u@g, !u@g
 *   %u list of REs, user must match at least one (^charon$,^entomb$)
 *   !u list of REs, user may never match any (^root$)
 *   %u@g list of REs, user must be a member of a matching group (^ops$)
 *   !u@g list of REs, user must not be a member of a matching group (^wheel$)
 */
static uid_t
MapUid(const char *pcThis, const const cmd_t *cmd, gid_t *pwGroup)
{
	register struct passwd *pwd;
	register char *pcLimit;

	if ((char *)0 != pcThis && '#' == *pcThis) {
		++pcThis;
	}
	if ((char *)0 == pcThis || '\000' == *pcThis) {
		fatal("mnemonic %s requires -u", cmd->pcname);
	}
	if (isdigit(*pcThis)) {
		if ((struct passwd *)0 == (pwd = getpwuid(atoi(pcThis)))) {
			fatal("%s: no such uid #%s", cmd->pcname, pcThis);
		}
	} else if ((struct passwd *)0 == (pwd = getpwnam(pcThis))) {
		fatal("%s: no such user %s", cmd->pcname, pcThis);
	}
	if ((gid_t *)0 != pwGroup) {
		*pwGroup = pwd->pw_gid;
	}
	/* apply any injunction against or a members-only list
	 */
	if ((char *)0 != (pcLimit = FindOpt(cmd, "!u"))) {
		if (AnyMatch(pcLimit, pwd->pw_name, pwd->pw_uid, "!u"))
			fatal("%s: login %s forbidden", cmd->pcname, pwd->pw_name);
	}
	if ((char *)0 != (pcLimit = FindOpt(cmd, "%u"))) {
		if (!AnyMatch(pcLimit, pwd->pw_name, pwd->pw_uid, "%u"))
			fatal("%s: login %s not in allowed list", cmd->pcname, pwd->pw_name);
	}
	if ((char *)0 != (pcLimit = FindOpt(cmd, "!u@g"))) {
		if (MemberMatch(pcLimit, pwd->pw_name, pwGroup, "!u@g"))
			fatal("%s: login %s black listed", cmd->pcname, pwd->pw_name);
	}
	if ((char *)0 != (pcLimit = FindOpt(cmd, "%u@g"))) {
		if (!MemberMatch(pcLimit, pwd->pw_name, pwGroup, "%u@g"))
			fatal("%s: login %s doesn't have a correct group", cmd->pcname, pwd->pw_name);
	}
	return pwd->pw_uid;
}

/* Check if the login list specified contains a matching login		(ksb)
 * When any RE is "", "%u" or "^%u$" match pcUid as a whole word, if %u is
 * a number reverse it to a login name first, if not given (!fUid) then you
 * can't match it anyway (skip the check).  Members format "a\000b\000\000".
 * Bonus %e for exec login (geteuid) and %l for original login (getuid),
 * since none of those are specail in a regular expression we'll pass Sanity.
 */
static int
MatchesMember(const char *pcRegList, const char *pcName, const char *pcMembers, const char *pcTag)
{
	register int fMatch, cWhich;
	register const char *pcCursor, *pcRE, *pcCheck, *pcWhom;
	register regex_t *reg;
	auto char *pcField;
	auto char acLoginRE[OP_MAX_LDWIDTH+4];	/* "^46600$" */

	fMatch = 0;
	for (pcRE = GetField(pcRegList, &pcField); 0 == fMatch && (char *)0 != pcRE; pcRE = GetField(pcRE, &pcField)) {
		register struct passwd *pwd;

		pcCheck = pcField;
		cWhich = 'u';
		if ('^' == *pcCheck) {
			++pcCheck;
		}
		if ('%' == *pcCheck && ('u' == pcCheck[1] || 'l' == pcCheck[1] || 'e' == pcCheck[1])) {
			cWhich = pcCheck[1];
			pcCheck += 2;
		}
		if ('$' == *pcCheck) {
			++pcCheck;
		}
		if ('\000' != *pcCheck) {
			if ((regex_t *)0 == (reg = ReComp(pcField, pcTag)))
				continue;
		} else {
			pcWhom = pcPerp;
			switch (cWhich) {
			case 'u':	/* -u target, only matches when given */
				if (!fUid)
					continue;
				pcWhom = pcUid;
				break;
			case 'l':	/* original login name */
				/* the default for -Wall flags */
				break;
			case 'e':	/* the setuid owner of op itself */
				snprintf(acLoginRE, sizeof(acLoginRE), "%ld", (long int)geteuid());
				pcWhom = acLoginRE;
				break;
			default:
				fatal("MapGid: %s: internal error", pcTag);
				/*NOTREACHED*/
			}
			if (isdigit(*pcWhom)) {
				if ((struct passwd *)0 == (pwd = getpwuid(atoi(pcWhom)))) {
					fatal("%s: no such uid #%s", pcTag, pcWhom);
				}
				snprintf(acLoginRE, sizeof(acLoginRE), "^%s$", pwd->pw_name);
				if ((regex_t *)0 == (reg = ReComp(acLoginRE, pcTag)))
					continue;
			} else {
				snprintf(acLoginRE, sizeof(acLoginRE), "^%s$", pcWhom);
				if ((regex_t *)0 == (reg = ReComp(acLoginRE, pcTag)))
					continue;
			}
		}

		for (pcCursor = pcMembers; '\000' != *pcCursor; pcCursor += strlen(pcCursor)+1) {
			if (ReMatch(reg, pcCursor)) {
				fMatch = 1;
				break;
			}
		}
		regfree(reg);
	}
	return fMatch;
}

/* Same thing for the group, as %g.					(ksb)
 *   %g list of REs, group must match at least one (^operator$,^disk$,)
 *   !g list of REs, group may never match any (^wheel$)
 *   %g@u list of REs, group must include at least one user that matches (^ksb$)
 *   !g@u list of REs, groups may never contain a login that matches any RE (^root$)
 * N.B. For g@u the login must be listed in the group file, not a member by
 * primary login group (because we check from the perspective of the group).
 *	It would be easy to scan the password file looking for logins with
 * the group in question, that would also be wrong for our (local) model.
 */
static gid_t
MapGid(char *pcThis, const cmd_t *cmd)
{
	register struct group *grp;
	register char *pcLimit, *pcBlackList, *pcWhiteList;
	register gid_t gRet;

	if ((char *)0 != pcThis && '#' == *pcThis) {
		++pcThis;
	}
	if ((char *)0 == pcThis || '\000' == *pcThis) {
		fatal("mnemonic %s requires -g", cmd->pcname);
	}
	if (isdigit(*pcThis)) {
		if ((struct group *)0 == (grp = getgrgid(atoi(pcThis)))) {
			fatal("%s: no such gid #%s", cmd->pcname, pcThis);
		}
		pcThis = strdup(grp->gr_name);
	} else if ((struct group *)0 == (grp = getgrnam(pcThis))) {
		fatal("%s: no such group %s", cmd->pcname, pcThis);
	}
	gRet = grp->gr_gid;
	if ((char *)0 != (pcLimit = FindOpt(cmd, "!g"))) {
		if (AnyMatch(pcLimit, pcThis, gRet, "!g"))
			fatal("%s: group %s blacklisted", cmd->pcname, pcThis);
	}
	if ((char *)0 != (pcLimit = FindOpt(cmd, "%g"))) {
		if (!AnyMatch(pcLimit, pcThis, gRet, "%g"))
			fatal("%s: group %s not in allowed list", cmd->pcname, pcThis);
	}

	/* Since MatchesMember might call getgrent(3) we can't hold the
	 * static buffer in libc when we call it, so save the member list.
	 * In the Scan loops below you'd add the setpwent loop if you
	 * wanted login group membership to count (which I think is wrong).
	 */
	pcBlackList = FindOpt(cmd, "!g@u");
	pcWhiteList = FindOpt(cmd, "%g@u");
	if ((char *)0 != pcWhiteList || (char *)0 != pcBlackList) {
		register const char *pcMembers;
		register char *pcMem, **ppcScan;
		register size_t wLen;

		wLen = 2;
		for (ppcScan = grp->gr_mem; (char *)0 != *ppcScan; ++ppcScan) {
			wLen += strlen(*ppcScan)+1;
		}
		pcMembers = pcMem = malloc((wLen|15)+1);
		for (ppcScan = grp->gr_mem; (char *)0 != *ppcScan; ++ppcScan) {
			(void)strcpy(pcMem, *ppcScan);
			pcMem += strlen(pcMem)+1;
		}
		*pcMem = '\000';
		if ((char *)0 != pcWhiteList && !MatchesMember(pcWhiteList, pcThis, pcMembers, "%g@u")) {
			fatal("%s: group %s doesn't have any matching login", cmd->pcname, pcThis);
		}
		if ((char *)0 != pcBlackList && MatchesMember(pcBlackList, pcThis, pcMembers, "!g@u")) {
			fatal("%s: group %s is blacklisted by membership", cmd->pcname, pcThis);
		}
		free((void *)pcMembers);
	}
	return gRet;
}

/* Return the highed explicit $n in the cmd's attributes list, for	(ksb)
 * counting how many command-line params participate in mnemonic matching.
 */
static long
MaxParams(const cmd_t *cmd)
{
	register int i;
	register const char *pcScan;
	register long lRet, lCur;
	auto char *pcRest;

	lRet = 0;
	for (i = 0; i < cmd->nopts; ++i) {
		pcScan = cmd->opts[i];
		if ('$' != pcScan[0] && '!' != *pcScan) {
			continue;
		}
		if (!isdigit(cmd->opts[i][1])) {
			continue;
		}
		if (0 == (lCur = strtoul(pcScan+1, & pcRest, 10))) {
			continue;
		}
		if (lCur > lRet)
			lRet = lCur;
	}
	return lRet;
}

/* Return 1 if every element in pcList is in pcListDup, given that	(ksb)
 * pcKeep is long enough to hold copy of any element from pcList.
 */
static int
HasEvery(const char *pcList, char *pcListDup, char *pcKeep)
{
	register int fSaw;
	register const char *pcScan;
	auto char *pcField, *pcFieldDup;

	while ((char *)0 != (pcList = GetField(pcList, &pcField))) {
		(void)strcpy(pcKeep, pcField);
		fSaw = 0;
		for (pcScan = GetField(pcListDup, &pcFieldDup); (char *)0 != pcScan; pcScan = GetField(pcScan, &pcFieldDup)) {
			if (0 != strcmp(pcKeep, pcFieldDup))
				continue;
			fSaw = 1;
			break;
		}
		if (!fSaw)
			return 0;
	}
	return 1;
}

/* Do the command have exactly the same $1, $2, $3... $n RE lists?	(ksb)
 * We don't try to see if the REs match the same text, I'm not quite that
 * crazy.  But we do look for some other obvious cases, like a permuted list.
 * We check for a leading zero botch in the main sanity checker (e.g. $01).
 * We comment (but do not fail) if two mnemonics are spelled the same from
 * two different files.
 */
static int
SamePats(const cmd_t *cmd, cmd_t *cmdDup)
{
	register long l, lMax, lMaxDup;
	register char *pcList, *pcListDup;
	register int fSaw;
	register char *pcMem;
	register size_t wLen;
	auto char acCvt[OP_MAX_LDWIDTH]; /* "$%ld" */

	lMax = MaxParams(cmd);
	lMaxDup = MaxParams(cmdDup);
	if (lMax < lMaxDup) {
		lMax = lMaxDup;
	}
	for (l = 1; l <= lMax; ++l) {
		if (sizeof(acCvt) <= snprintf(acCvt, sizeof(acCvt), "$%ld", l)) {
			fatal("%s: $%ld buffer overflow", cmd->pcname, l);
		}
		pcList = FindOpt(cmd, acCvt);
		pcListDup = FindOpt(cmdDup, acCvt);
		if ((char *)0 == pcList && (char *)0 == pcListDup)
			continue;
		if ((char *)0 == pcList || (char *)0 == pcListDup)
			return 0;

		/* The two lengths or'd must be at least as big as either
		 * or in 017 and add 1 for the \000 on the end
		  */
		wLen = (strlen(pcList)|strlen(pcListDup)|15)+1;
		if ((char *)0 == (pcMem = malloc(wLen))) {
			fatal(acBadAlloc);
		}
		fSaw = HasEvery(pcList, pcListDup, pcMem) && HasEvery(pcListDup, pcList, pcMem);
		free((void *)pcMem);
		if (!fSaw)
			return 0;
	}
	return 1;
}

/* Trusting the client user's environment might be a Bad Thing.		(ksb)
 */
static void
CheckTrust(const cmd_t *cmd, char *pcCheck)
{
	register char *pcCur, *pcList;

	if ((char *)0 != (pcCur = FindOpt(cmd, pcCheck)) && '\000' != *pcCur)
		return;
	if ((char *)0 != pcCur || ((char *)0 != (pcList = FindOpt(cmd, "environment")) && ('\000' == *pcList || GenMatch("environment", "PATH", pcList) || GenMatch("environment", "PATH=/tmp/any", pcList))))
		fprintf(stderr, "%s: %s: %s:%d: trusting the %s from the user's environment\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcCheck);
}

/* Find any RE in the list that doesn't compile.			(ksb)
 */
static char *
CheckCompile(const char *pcList, const char *pcName)
{
	register regex_t *reg;
	register char *pcRet;
	auto char *pcField;

	if ((char *)0 == pcList || '\000' == *pcList) {
		return (char *)0;
	}
	pcRet = (char *)0;
	while ((char *)0 != (pcList = GetField(pcList, &pcField))) {
		if ((regex_t *)0 != (reg = ReComp(pcField, (char *)0))) {
			regfree(reg);
			continue;
		}
		pcRet = pcField;
		break;
	}
	return pcRet;
}

/* Convert the RE into a usage strings (which must be shorter than the	(ksb)
 * original RE or the word "number"). The expression must be anchored.
 * An expression is easy to convert to a usage:
 *	"^start$" => "start",
 *	"^(start|stop|end)$" or
 *	"^start$|^stop$|^end$" => "start|stop|end"
 *	"(^start$)|(^stop)$" => start|stop
 * nested parens stop use because we don't expand them:
 *	"^bl(y|ue|t|x)$" => default since "bly|blue|blt|blx" is longer
 * For more complex things return the default.  We scribble over the RE
 * to prodce a suggested usage message.
 */
static const char *
IsAnchored(char *pcRE, const char *pcDef)
{
	register size_t i, iOut, iLen;
	register int fParens, iDots, iSlash;
	register const char *pcCheck;

	fParens = 0;
	if ('(' == pcRE[0] && '^' == pcRE[1]) {
		fParens = 1;
		pcRE += 2;
	} else if ('^' != pcRE[0]) {
		return pcDef;
	} else if ('(' == pcRE[1]) {
		fParens = 2;
		pcRE += 2;
	} else {
		++pcRE;
	}
	if (0 == (iLen = strlen(pcRE))) {
		return pcDef;
	}
	switch (fParens) {
	case 1:	/* "(^ .... $)" */
		if (/*(*/ ')' != pcRE[--iLen])
			return pcDef;
		/*FALLTHROUGH*/
	case 0:	/* ^foo$ */
		if ('$' !=  pcRE[--iLen])
			return pcDef;
		break;
	case 2:	/* "^( ... )$ */
		if ('$' != pcRE[--iLen])
			return pcDef;
		if (/*(*/ ')' != pcRE[--iLen])
			return pcDef;
		break;
	}

	/* A pattern that matches all digits is a "number"
	 *	digit | '[' digit ']' | '[' digit '-' digit ']' | '*' | '+'
	 * so we accept '**1' as a number: don't give us a bad RE, we also
	 * can match an IP address (a number with 3 dots in it) or a CIDR.
	 */
	iOut = 1;
	iSlash = iDots = 0;
	for (pcCheck = pcRE, i = 0; i < iLen; ) {
		pcCheck = pcRE+i;
		if (isdigit(*pcCheck))
			i += 1;
		else if ('/' == *pcCheck)
			iOut = 0, i += 1, iSlash += 1;
		else if ('\\' == pcCheck[0] && '/' == pcCheck[1])
			iOut = 0, i += 2, iSlash += 1;
		else if ('+' == *pcCheck || '*' == *pcCheck)
			iOut = 0, i += 1;
		else if ('\\' == pcCheck[0] && '.' == pcCheck[1])
			iOut = 0, iDots += 1, i += 2;
		else if ('[' != pcCheck[0] || ']' == pcCheck[1])
			break;
		else {	/* [0-9] | [.0-9] | [0-9.] | [234] */
			register int iSawDot = 0;

			do {
				++i;
				if ('-' == pcRE[i] || isdigit(pcRE[i]))
					/* nada */;
				else if ('/' == pcRE[i])
					iSlash += 1;
				else if ('.' == pcRE[i])
					iSawDot = 1;
				else
					break;
			} while (']' != pcRE[i]);
			if (']' != pcRE[i])
				break;
			if (iSawDot) {
				iDots = '*' == pcRE[i+1] || '+' == pcRE[i+1] ? 3 : iDots+1;
			}
			++i, iOut = 0;
		}
	}
	if (i == iLen) {
		pcRE[iLen] = '\000';
		return iOut ? pcRE : strcpy(pcRE, 3 == iDots ? 1 == iSlash ? "CIDR" : "IP" : "number");
	}
	iOut = 0;
	for (i = 0; i < iLen; /* below */) {
		switch ((pcRE[iOut++] = pcRE[i++])) {
		case '(':
		case ')': /* assume balanced if -S ran, allow it */
			continue;
		case '$':
			switch (fParens) {
			case 0:
			case 1:
				if ('|' != pcRE[i] || '^' != pcRE[i+1])
					return pcDef;
				i += 2;
				pcRE[iOut-1] = '|';
				continue;
			case 2:	/* a literal dollar */
				continue;
			}
		case '|':	/* is ok if in parens, type 2 */
			/* ^foo|bar$ -S will pitch about missing anchors
			 * should be ^foo$|^bar$, but allow it: carping at
			 * the Customer won't help anyone.
			 */
			continue;
		case '*': case '+': case '?':
		case '.': case '[': case ']':
		case '{': case '}':
			return pcDef;
		case '\\':
			if (i == iLen)	/* missparse the $, its a \$ */
				return pcDef;
			if (isdigit(pcRE[i]))
				return pcDef;
			pcRE[iOut-1] = pcRE[i];
			continue;
		case ',':	/* outside of {a,b} is ok */
		default:
			continue;
		}
	}
	pcRE[iOut] = '\000';
	return pcRE;
}

/* Compile the REs in the list return 1 if any match a string in data.	(ksb)
 * Vetch about things we don't like, and for non-zero exits on errors.
 */
static int
CheckMatch(const cmd_t *cmd, const char *pcWhich, char **ppcData, const char *pcList, int *piRet)
{
	register int i, iMatch;
	register int fSawAnchor, fAllAnchors;
	register regex_t *reg;
	register const char *pcCheck;
	auto char *pcField;

	iMatch = 0;
	fSawAnchor = 0;
	fAllAnchors = 1;
	while ((char *)0 != (pcList = GetField(pcList, &pcField))) {
		if ('\000' == *pcField) {
			fprintf(stderr, "%s: %s: %s:%d: %s: empty RE\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcWhich);
			*piRet = EX_DATAERR;
			continue;
		}
		if ((regex_t *)0 == (reg = ReComp(pcField, (const char *)0))) {
			fprintf(stderr, "%s: %s: %s:%d: %s: \"%s\"\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcWhich, pcField);
			*piRet = EX_DATAERR;
			continue;
		}
		/* A # pattern matches a numeric form of a login/group, for
		 * a netgroup it doesn't, but we're not perfect and a hash
		 * can't really ever be in a netgroup name as it is a comment.
		 */
		if ('#' == *pcField) {
			++pcField;
		}
		pcCheck = IsAnchored(pcField, (0 == strcmp(pcField, ".*") || 0 == strcmp(pcField, "^.*$")) ? acDevNull : (char *)0);
		fAllAnchors &= (const char *)0 != pcCheck;
		fSawAnchor |= (const char *)0 != pcCheck;
		for (i = 0; (char *)0 != ppcData[i]; ++i) {
			if (ReMatch(reg, ppcData[i])) {
				iMatch = 1;
				break;
			}
		}
		regfree(reg);
	}
	if (!fSawAnchor) {
		fprintf(stderr, "%s: %s: %s:%d: %s: no anchors in any REs%s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcWhich, acSure);
		/* might be worth a non-zero exit */
	} else if (!fAllAnchors) {
		fprintf(stderr, "%s: %s: %s:%d: %s: some REs unanchored%s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcWhich, acSure);
		/* not be worth a non-zero exit */
	}

	return iMatch;
}

/* See if this program measures up to our standards.			(ksb)
 */
static void
CheckProg(cmd_t *cmd, const char *pcProg, int *piRet)
{
	auto struct stat stProg;

	if (acDefault == cmd->pcname) {
		return;
	}
	if (0 != stat(pcProg, & stProg)) {
		fprintf(stderr, "%s: %s: %s:%d: stat: %s: %s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcProg, strerror(errno));
		*piRet = EX_NOINPUT;
	} else if (0 == (stProg.st_mode&0111)) {
		fprintf(stderr, "%s: %s: %s:%d: %s: not executable\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcProg);
		*piRet = EX_DATAERR;
	} else if ('/' != pcProg[0]) {
		fprintf(stderr, "%s: %s: %s:%d: %s: should be an absolute path\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcProg);
		*piRet = EX_PROTOCOL;
	} else if (0 != (stProg.st_mode&0002)) {
		fprintf(stderr, "%s: %s: %s:%d: %s: world writable!\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcProg);
		*piRet = EX_NOPERM;
	}
}

/* Check a stdin, stdout, stderr redir for sanity.			(ksb)
 * See PrivOpen for specs (below).
 */
static int
CheckRedir(const cmd_t *cmd, char *pcRedirExpected, char *pcAttr)
{
	register char *pcMode, *pcSpec;
	auto struct stat stCheck;

	if ((char *)0 == (pcSpec = FindOpt(cmd, pcAttr))) {
		return 1;
	}
	if ('<' == *pcSpec || '>' == *pcSpec) {
		pcMode = pcSpec;
		if (*pcMode != *pcRedirExpected) {
			fprintf(stderr, "%s: %s: %s:%d: %s: I/O direction is not standard (%c presented, %c expected)%s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcAttr, *pcMode, *pcRedirExpected, acSure);
			/* not worth a non-zero exit code */
		}
		if ('>' == *++pcSpec)
			++pcSpec;
	} else {
		pcMode = pcRedirExpected;
	}
	if (0 == strcmp("%f", pcSpec)) {
		return 1;
	}
	if ('/' == *pcSpec) {
		if (-1 == stat(pcSpec, &stCheck)) {
			fprintf(stderr, "%s: %s: %s:%d: %s: stat: %s: %s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcAttr, pcSpec, strerror(errno));
			return 0;
		}
		/* don't check for type, etc */
		return 1;
	}
	if ((char *)0 == FindOpt(cmd, "dir")) {
		fprintf(stderr, "%s: %s: %s:%d: %s is relative to $PWD\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcAttr);
		/* not worth a non-zero exit code */
	}
	return 0;
}

/* Verify that each configuration option in this cmd is unique		(ksb)
 */
static void
CheckUniq(const cmd_t *cmd, int *piRet)
{
	register int i, j;
	register size_t wLen;
	register const char *pcEq, *pcI, *pcJ;

	for (i = 0; i < cmd->nopts; ++i) {
		pcI = cmd->opts[i];
		if ((const char *)0 == (pcEq = strchr(pcI, '=')))
			wLen = strlen(pcI);
		else
			wLen = pcEq - pcI;
		for (j = i+1; j < cmd->nopts; ++j) {
			pcJ = cmd->opts[j];
			if (0 != strncmp(pcI, pcJ, wLen))
				continue;
			if ('\000' != pcJ[wLen] && '=' != pcJ[wLen])
				continue;
			fprintf(stderr, "%s: %s: %s:%d: %.*s given more than once\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, wLen, pcI);
			*piRet = EX_CONFIG;
			return;
		}
	}
}

/* Netgroups, pam application names and such are not specified as	(ksb)
 * regular expressions, but some people try that.  Check them at the door.
 * N.B. A single dot (acMyself) can't look like an RE to this function.
 */
static int
LooksLikeRE(const char *pcThis)
{
	return ((char *)0 != strchr(pcThis, '^') ||
		(char *)0 != strchr(pcThis, '$') ||
		(char *)0 != strchr(pcThis, '['/*]*/) ||
		(char *)0 != strchr(pcThis, '('/*)*/));
}

/* Sanity check our rule database.				(ksb/petef)
 * We've read the default configuration files, what would break
 * if we added the others?
 */
static void
SanityCheck(int argc, char **argv)
{
	register int iIndex;
	register char *pcUser, *pcGroup, **ppcGroups, **ppcUsers;
	auto int iRet;
	auto NEEDS Need;

	iRet = EX_OK;
	if (0 != argc) {
		register int uOrig, uPriv, fdRead;
		uOrig = getuid();
		uPriv = geteuid();

		if (setreuid(uPriv, uOrig) < 0) {
			fatal("setreuid: %s", strerror(errno));
		}
		for (iIndex = 0; iIndex < argc; ++iIndex) {
			if (-1 == (fdRead = open(argv[iIndex], O_RDONLY, 0400))) {
				fatal("open: %s: %s", argv[iIndex], strerror(errno));
			}
			OneFile(fdRead, argv[iIndex]);
		}
		/* If we put the euid back then anyone can stat(2) any file on
		 * our filesystem for existance.  Radiates too much info. --ksb
		 */
	}

	/* yuck: we have to get a cache of all possible users + groups to
	 * make sure that users= and groups= matches something
	 */
	{
	register struct passwd *ppassThis;
	register struct group *pgroupThis;
	register int iMax;

	setgrent();
	iMax = 512;
	iIndex = 0;
	if ((char **)0 == (ppcGroups = (char **)calloc(iMax, sizeof(char *))))
		fatal(acBadAlloc);
	while ((struct group *)0 != (pgroupThis = getgrent())) {
		ppcGroups[iIndex++] = strdup(pgroupThis->gr_name);
		if (iMax == iIndex) {
			iMax *= 2;
			ppcGroups = (char **)realloc(ppcGroups, iMax*sizeof(char *));
			if ((char **)0 == ppcGroups)
				fatal(acBadAlloc);
		}
	}
	ppcGroups[iIndex] = (char *)0;
	endgrent();

	setpwent();
	iMax = 512;
	iIndex = 0;
	if ((char **)0 == (ppcUsers = (char **)calloc(iMax, sizeof(char *))))
		fatal(acBadAlloc);
	while ((struct passwd *)0 != (ppassThis = getpwent())) {
		ppcUsers[iIndex++] = strdup(ppassThis->pw_name);
		if (iMax == iIndex) {
			iMax *= 2;
			ppcUsers = (char **)realloc(ppcUsers, iMax*sizeof(char *));
			if ((char **)0 == ppcUsers)
				fatal(acBadAlloc);
		}
	}
	endpwent();
	}

	/* Sanity check complied options
	 */
	{
#if USE_PAM
	static const char acPam[] = OP_PAM_IN;
	auto struct stat stCheck;
#endif

	if ('/' != acAccess[0]) {
		fprintf(stderr, "%s: configuration path is not absolute (%s)\n", Progname, acAccess);
		iRet = EX_CONFIG;
	}
#if USE_PAM
	if (-1 == stat(acPam, &stCheck) || S_IFDIR != (stCheck.st_mode & S_IFMT)) {
		fprintf(stderr, "%s: PAM configured but no pam configuration directory (%s)\n", Progname, acPam);
		iRet = EX_CONFIG;
	}
#endif
	}

	/* Sanity check each command
	 */
	{
	register cmd_t *cmd, *cmdDup, *cmdScan;
	register char *pcCur, *pcDup, *pcDir;
	register const char *pcRead;
	register long lCur, lDup;
	auto char *pcEnd, *pcField;
	auto int fSpecF, fSpecG, fSpecU, iMatch;
	auto struct stat statThis;
	auto char acToExec[MAXPATHLEN+256];

	cmd = (cmd_t *)0;
	for (cmdScan = NewCmd((const char *)0); (cmd_t *)0 != cmdScan; cmdScan = cmdScan->pODnext) {
		/* a DEFAULT cannot have $#, $1, $2... because we check
		 * them before we Build the command
		 */
		if (acDefault == cmdScan->pcname) {
			for (iIndex = 0; iIndex < cmdScan->nopts; ++iIndex) {
				pcRead = cmdScan->opts[iIndex];
				if ('$' != pcRead[0] && '!' != pcRead[0]) {
					continue;
				}
				if ('#' == pcRead[1]) {
					fprintf(stderr, "%s: %s:%d: %s: %c# is not a valid default attribute\n", Progname, cmdScan->pcfile, cmdScan->iline, cmdScan->pcname, pcRead[0]);
					iRet = EX_CONFIG;
					continue;
				}
				if (!isdigit(pcRead[1])) {
					continue;
				}
				fprintf(stderr, "%s: %s:%d: %s: %c", Progname, cmdScan->pcfile, cmdScan->iline, cmdScan->pcname, pcRead[0]);
				while (isdigit(*++pcRead)) {
					fputc(*pcRead, stderr);
				}
				fprintf(stderr, " is not a valid default attribute\n");
				iRet = EX_CONFIG;
			}
			if (0 != iIndex && (cmd_t *)0 != cmdScan->pODnext && acDefault == cmdScan->pODnext->pcname && (cmd_t *)0 != cmd) {
				fprintf(stderr, "%s: %s: %s:%d non-empty applies to no rules\n", Progname, acDefault, cmdScan->pcfile, cmdScan->iline);
				/* not worth a non zero exit code */
			}
		}

		/* Make sure all options are unique, we don't process
		 * duplicate options well.
		 */
		CheckUniq(cmdScan, & iRet);

		/* We have to check all merged DEFAULT options
		 */
		cmd = Build(cmdScan);

		/* If there is no args[0] then it's a syntax error, only
		 * happens at the end-of-file when missing all args
		 */
		if (0 == cmd->nargs && acDefault != cmd->pcname) {
			static int fTold = 0;

			if (! fTold) {
				fprintf(stderr, "%s: %s:%d: %s: configuration file syntax error (%s)\n", Progname, cmd->pcfile, cmd->iline, cmd->pcname, acDefault == cmd->pcname ? "the default option must be first" : "file truncated?");
				fTold = 1;
			}
			iRet = EX_CONFIG;
			continue;
		}
		if (0 != cmd->nopts) {
			/* ok */
		} else if (acDefault == cmd->pcname) {
			/* This is the documented way to remove the
			 * DEFAULT in access.cf, so not an error. --ksb
			 */
		} else {
			register int iLook;

			for (iLook = 0; iLook < cmd->nargs; ++iLook) {
				if ((char *)0 != strchr(cmd->args[iLook], '='))
					break;
			}
			fprintf(stderr, "%s: %s:%d: %s: a missing semicolon may have consumed all options", Progname, cmd->pcfile, cmd->iline, cmd->pcname);
			if (0 != iLook && iLook < cmd->nargs) {
				fprintf(stderr, " (maybe after \"%s\")", cmd->args[iLook-1]);
			}
			fprintf(stderr, "\n");
		}
		lCur = -2;
		if ((char *)0 != (pcCur = FindOpt(cmd, "$#"))) {
			lCur = strtol(pcCur, &pcEnd, 0);
			if ((char *)0 != pcEnd && '\000' != *pcEnd) {
				fprintf(stderr, "%s: %s: %s:%d: cannot parse count in \"$#=%s\"\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcCur);
				lCur = -1;
			} else if (0 > lCur) {
				fprintf(stderr, "%s: %s: %s:%d: $# value %ld won't match any mnemonic rules\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, lCur);
			}
		}
		for (cmdDup = cmd->pODnext; (cmd_t *)0 != cmdDup && acDefault != cmd->pcname; cmdDup = cmdDup->pODnext) {
			if (0 != strcmp(cmd->pcname, cmdDup->pcname))
				continue;

			if (!cmd->fcarp && 0 != strcmp(cmd->pcfile, cmdDup->pcfile)) {
				fprintf(stderr, "%s: %s should only be defined in a single configuration file (not both %s and %s)\n", Progname, cmd->pcname, cmd->pcfile, cmdDup->pcfile);
				cmd->fcarp = 1;
			}
			pcDup = FindOpt(cmdDup, "$#");
			if ((char *)0 == pcCur && (char *)0 == pcDup) {
				if (SamePats(cmd, cmdDup))
					fprintf(stderr, "%s: %s: duplicated in %s:%d and %s:%d\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, cmdDup->pcfile, cmdDup->iline);
				continue;
			}
			if ((char *)0 == pcCur || (char *)0 == pcDup) {
				continue;
			}
			if (lCur != (lDup = strtol(pcDup, &pcEnd, 0))) {
				continue;
			}
			if (SamePats(cmd, cmdDup)) {
				fprintf(stderr, "%s: %s: duplicated in %s:%d and %s:%d, argument count %ld\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, cmdDup->pcfile, cmdDup->iline, lDup);
			}
		}

		/* If the command is magic shell, check $SHELL.
		 * Params are the format of the shell call with $S as the
		 * shell, $* is the command-line as a word the default
		 * template is "$S" "-c" "$*" (at /"-c" below).
		 */
		pcRead = (0 == cmd->nargs) ? acDevNull : cmd->args[0];
		if (0 == strcmp(acMagicShell, pcRead)) {
			if (1 == cmd->nargs || 0 == strcmp("$S", cmd->args[1]))
				CheckTrust(cmd, "$SHELL");
			pcRead = acDefShell;
		}
		/* Check sanity check the args templates as best we can
		 */
		fSpecF = fSpecG = fSpecU = 0;
		if (0 != Craving(cmd, & Need)) {
			iRet = EX_DATAERR;
		}
		if ((char *)0 == (pcDir = FindOpt(cmd, "chroot")) || 0 == strcmp("%d", pcDir) || 0 == strcmp("%f", pcDir)) {

			/* don't check it */
		} else {
			if ('\000' == pcDir)
				pcDir = "/";
			if ('/' != pcDir[0]) {
				fprintf(stderr, "%s: %s: %s:%d: chroot: %s: is not an absolute path\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcDir);
				iRet = EX_CONFIG;
			}
			if (-1 == stat(pcDir, & statThis)) {
				fprintf(stderr, "%s: %s: %s:%d: chroot=%s: %s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcDir, strerror(errno));
				iRet = EX_NOINPUT;
			} else if ((statThis.st_mode & S_IFMT) != S_IFDIR) {
				fprintf(stderr, "%s: %s: %s:%d: chroot=%s: not a directory\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcDir);
				iRet = EX_CONFIG;
			}
		}
		if ((char *)0 == (pcDir = FindOpt(cmd, "dir")) || 0 == strcmp("%d", pcDir) || 0 == strcmp("%f", pcDir)) {
			pcDir = "";	/* becomes "/" inv. below */
		} else {
			if ('\000' == pcDir)
				pcDir = "/";
			if ('/' != pcDir[0]) {
				fprintf(stderr, "%s: %s: %s:%d: dir: %s: is not an absolute path\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcDir);
				iRet = EX_CONFIG;
			}
			if (-1 == stat(pcDir, & statThis)) {
				fprintf(stderr, "%s: %s: %s:%d: dir=%s: %s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcDir, strerror(errno));
				iRet = EX_NOINPUT;
			} else if ((statThis.st_mode & S_IFMT) != S_IFDIR) {
				fprintf(stderr, "%s: %s: %s:%d: dir=%s: not a directory\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcDir);
				iRet = EX_CONFIG;
			} else if ('/' != *pcRead && 0 != (statThis.st_mode & 0002)) {
				fprintf(stderr, "%s: %s: %s:%d: dir=%s: world write on directory could allow Trojan version of \"%s\"\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcDir, pcRead);
			}
		}
		for (;;) { switch (pcRead[0]) {
		case '/':	/* we are an absolute path, so no prefix */
			pcDir = "";
			break;
		case '.':	/* remove redundant ./ prefixes */
			if ('/' != pcRead[1])
				break;
			do
				++pcRead;
			while ('/' == pcRead[0]);
			break;
		case '\000':
			fprintf(stderr, "%s: %s: %s:%d: null command?\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline);
			iRet = EX_CONFIG;
			break;
		default:
			/* Are we trusting $PATH? */
			CheckTrust(cmd, "$PATH");
			pcDir = (char *)0;
			break;
		} break; }
		if ((char *)0 != pcDir && MAXPATHLEN-1 < strlen(pcDir)+strlen(pcRead)) {
			fprintf(stderr, "%s: %s: %s:%d: directory + exec path too long\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline);
			iRet = EX_CONFIG;
			continue;
		}
		if ((char *)0 != pcDir) {
			(void)strcpy(acToExec, pcDir);
			pcDir = acToExec+strlen(acToExec);
			while (pcDir > acToExec && '/' == pcDir[-1]) {
				--pcDir;
			}
			if ('/' != pcRead[0]) {
				*pcDir++ = '/';
			}
			(void)strcpy(pcDir, pcRead);
		}
		/* make sure we can stat the command we're going to run */
		if ((char *)0 == pcDir) {
			/* don't look */
		} else if (0 == stat(acToExec, & statThis)) {
			CheckProg(cmd, acToExec, &iRet);
		} else if ((char *)0 != (pcDir = strrchr(acToExec, '$'))) {
			/* Make sure the directory the foo.$1 file is in
			 * exists.  Even if $1 has a slash in it, the base
			 * still should exist.
			 */
			do {
				*pcDir-- = '\000';
			} while (pcDir > acToExec && '/' != *pcDir);
			if (-1 == stat(acToExec, & statThis)) {
				fprintf(stderr, "%s: %s: %s:%d: computed path to directory %s: %s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, acToExec, strerror(errno));
				iRet = EX_NOINPUT;
			}
		} else {
			fprintf(stderr, "%s: %s: %s:%d: %s: %s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, acToExec, strerror(errno));
			iRet = EX_NOINPUT;
		}

		/* Give up because we'll not match any FindOpts below, and
		 * we don't need any options to work
		 */
		if (0 == cmd->nopts && !Need.fUid && !Need.fGid && !Need.fSecFile) {
			continue;
		}

		/* check to see we have a valid uid= and gid= */
		if ((char *)0 == (pcUser = FindOpt(cmd, "uid"))) {
			if (acDefault != cmd->pcname)
				fprintf(stderr, "%s: %s: %s:%d: uid defaults to the superuser, make that explicit\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline);
		} else if (0 != strcmp(pcUser, acMyself) && 0 != strcmp(pcUser, "%u") && 0 != strcmp(pcUser, "%f")) {
			if (isdigit(*pcUser) && (struct passwd *)0 == getpwuid(atoi(pcUser))) {
				fprintf(stderr, "%s: %s: %s:%d: no such uid %s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcUser);
				iRet = EX_NOUSER;
			}
			if (!isdigit(*pcUser) && (struct passwd *)0 == getpwnam(pcUser)) {
				fprintf(stderr, "%s: %s: %s:%d: no such user %s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcUser);
				iRet = EX_NOUSER;
			}
		}
		if ((char *)0 != (pcUser = FindOpt(cmd, "euid")) && 0 != strcmp(pcUser, acMyself) && 0 != strcmp("%u", pcUser) && 0 != strcmp(pcUser, "%f")) {
			if (isdigit(*pcUser) && (struct passwd *)0 == getpwuid(atoi(pcUser))) {
				fprintf(stderr, "%s: %s: %s:%d: no such euid %s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcUser);
				iRet = EX_NOUSER;
			}
			if (!isdigit(*pcUser) && (struct passwd *)0 == getpwnam(pcUser)) {
				fprintf(stderr, "%s: %s: %s:%d: no such effective user %s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcUser);
				iRet = EX_NOUSER;
			}
		}
		if ((char *)0 != (pcRead = FindOpt(cmd, "initgroups")) && 0 != strcmp("%u", pcRead) && 0 != strcmp(pcRead, "%f") && 0 != strcmp(pcRead, acMyself)) {
			if ((char *)0 != strchr(pcRead, ',')) {
				fprintf(stderr, "%s: %s: %s:%d: initgroups %s: is not a list -- just a single login name\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcRead);
				iRet = EX_CONFIG;
			}
			if ('\000' == *pcRead) {
				if ((char *)0 == FindOpt(cmd, "uid") && (char *)0 == FindOpt(cmd, "euid")) {
					fprintf(stderr, "%s: %s: %s:%d: initgroups %s: empty value needs \"uid\" or \"euid\" set\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcRead);
					iRet = EX_CONFIG;
				}
			} else if (isdigit(*pcRead)) {
				fprintf(stderr, "%s: %s: %s:%d: initgroups %s: must be a login name, %%f, or %%u\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcRead);
				iRet = EX_NOUSER;
			} else if ((struct passwd *)0 == getpwnam(pcRead)) {
				fprintf(stderr, "%s: %s: %s:%d: no such initgroups login %s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcRead);
				iRet = EX_NOUSER;
			}
		}
		/* N.B. we use pcDup to avoid fetching the session again below
		 */
		if ((char *)0 != (pcRead = pcDup = FindOpt(cmd, acOptSession)) && '\000' != *pcRead && 0 != strcmp("%u", pcRead) && 0 != strcmp(pcRead, "%i") && 0 != strcmp(pcRead, "%f") && 0 != strcmp(pcRead, acMyself)) {
			if ((char *)0 != strchr(pcRead, ',')) {
				fprintf(stderr, "%s: %s: %s:%d: session %s: is not a list -- just a single login specification\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcRead);
				iRet = EX_CONFIG;
			} else if (isdigit(*pcRead)) {
				fprintf(stderr, "%s: %s: %s:%d: session %s: must be a login name, %%f, %%i, %%u, or %s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcRead, acMyself);
				iRet = EX_NOUSER;
			} else if ((struct passwd *)0 == getpwnam(pcRead)) {
				fprintf(stderr, "%s: %s: %s:%d: no such session login %s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcRead);
				iRet = EX_NOUSER;
			}
		}
		if ((char *)0 != (pcRead = FindOpt(cmd, acOptCleanup)) && '\000' != *pcRead && 0 != strcmp(acMyself, pcRead)) {
			if ((char *)0 != strchr(pcRead, ',')) {
				fprintf(stderr, "%s: %s: %s:%d: %s %s: is not a list -- just a single application specification or \"%s\"\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, acOptCleanup, pcRead, acMyself);
				iRet = EX_CONFIG;
			} else if (LooksLikeRE(pcRead)) {
				fprintf(stderr, "%s: %s: %s:%d: %s %s: looks like a regular expression, should be a PAM application\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, acOptCleanup, pcRead);
				iRet = EX_CONFIG;
			} else if ((char *)0 == pcDup || '\000' == *pcDup) {
				fprintf(stderr, "%s: %s: %s:%d: %s %s: no matching %s specified\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, acOptCleanup, pcRead, acOptSession);
				/* not worth the non-zero exit code */
			}
		}
		if ((char *)0 != (pcRead = FindOpt(cmd, "gid"))) {
			while ((char *)0 != (pcRead = GetField(pcRead, &pcField))) {
				if (0 == strcmp(acMyself, pcField)) {
					/* nada */
				} else if (0 == strcmp("%f", pcField)) {
					/* nada */
				} else if (0 == strcmp("%u", pcField)) {
					/* nada */
				} else if (0 == strcmp("%g", pcField)) {
					/* nada */
				} else if (isdigit(*pcField)) {
					if ((struct group *)0 == getgrgid(atoi(pcField))) {
						fprintf(stderr, "%s: %s: %s:%d: no such gid %s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcField);
						iRet = EX_NOUSER;
					}
				} else if ((struct group *)0 == getgrnam(pcField)) {
					fprintf(stderr, "%s: %s: %s:%d: no such group %s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcField);
					iRet = EX_NOUSER;
				}
			}
		}
		if ((char *)0 != (pcGroup = FindOpt(cmd, "egid"))) {
			if (0 == strcmp(acMyself, pcGroup)) {
				/* nada */
			} else if (0 == strcmp("%f", pcGroup)) {
				/* force -f */
			} else if (0 == strcmp("%u", pcGroup)) {
				/* force -u */
			} else if (0 == strcmp("%g", pcGroup)) {
				/* force -g */
			} else if (isdigit(*pcGroup)) {
				if ((struct group *)0 == getgrgid(atoi(pcGroup))) {
					fprintf(stderr, "%s: %s: %s:%d: no such egid %s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcGroup);
					iRet = EX_NOUSER;
				}
			} else if ((struct group *)0 == getgrnam(pcGroup)) {
				fprintf(stderr, "%s: %s: %s:%d: no such effective group %s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcGroup);
				iRet = EX_NOUSER;
			}
		}

		/* Look for any posible groups, users, that might match this
		 * rule and carp when there is not even a possible match, also
		 * look through each specified netgroup, try to guess if this
		 * group might match someone.  If we match the host and the
		 * host allows anyone in the current login list, for example.
		 * We don't look at domain.  We do debug your /etc/netgroup.
		 */
		iMatch = 0;
		if ((char *)0 != (pcGroup = FindOpt(cmd, acOptGroups))) {
			iMatch |= CheckMatch(cmd, acOptGroups, ppcGroups, pcGroup, &iRet);
		}
		if ((char *)0 != (pcRead = FindOpt(cmd, acOptUsers))) {
			iMatch |= CheckMatch(cmd, acOptUsers, ppcUsers, pcRead, &iRet);
		}
		if (((char *)0 != (pcGroup = FindOpt(cmd, acOptNetgroups)))) {
			auto char *pcHost, *pcLogin, *pcDomain;
			auto char acMyName[512]; /* traditionally 256	*/
			auto char acPat[128]; /* temp RE: m/^$login$/	*/

			if (-1 == gethostname(acMyName, sizeof(acMyName))) {
				(void)strncpy(acMyName, ":", sizeof(acMyName));
			}
			pcRead = pcGroup;
			while ((char *)0 != (pcRead = GetField(pcRead, &pcField))) {
				if (LooksLikeRE(pcField)) {
					fprintf(stderr, "%s: %s: %s:%d: netgroup %s: looks like a regular expression, should be a name\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcField);
					/* is not 100% sure, don't fail */
				}
				if ('-' == pcField[0] && '\000' == pcField[1]) {
					continue;
				}
				setnetgrent(pcField);
				while (getnetgrent(&pcHost, &pcLogin, &pcDomain)) {
					pcGroup = (char *)0;
					if ((char *)0 != pcHost && 0 != strcmp(acMyName, pcHost)) {
						if ('*' == pcHost[0]) {
							fprintf(stderr, "%s: netgroup %s: use the empty field for any host (not an explicit '*')\n", Progname, pcField);
							continue;
						}
						continue;
					}
					if ((char *)0 == pcLogin) {
						iMatch |= 1;
						continue;
					}
					if ('*' == pcLogin[0]) {
						fprintf(stderr, "%s: netgroup %s: use the empty field for anyone (not an explicit '*')\n", Progname, pcField);
						continue;
					}
					snprintf(acPat, sizeof(acPat), "^%s$", pcLogin);
					iMatch |= CheckMatch(cmd, acOptNetgroups, ppcUsers, acPat, &iRet);
				}
				endnetgrent();
			}
			if ((char *)0 != pcGroup) {
				fprintf(stderr, "%s: %s: %s:%d none of these netgroups exist: %s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcGroup);
				/* not worth a nonzero exit code */
			}
		}
		if (((char *)0 != (pcRead = FindOpt(cmd, "helmet")))) {
			CheckProg(cmd, pcRead, &iRet);
		}
		if (((char *)0 != (pcRead = FindOpt(cmd, "jacket")))) {
			CheckProg(cmd, pcRead, &iRet);
		}
		if (acDefault != cmd->pcname && 0 == iMatch) {
			fprintf(stderr, "%s: %s: %s:%d: no users, groups, or netgroups match\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline);
			iRet = EX_NOUSER;
		}
		if ((char *)0 != (pcDup = FindOpt(cmd, "fib"))) {
			register long lFib;
			auto int iMaxFib = 1;
#if HAVE_SETFIB
			auto int aiMib[16];
			auto size_t iMibLen, iDataLen;
			static const char acNetFibs[] = "net.fibs";
#endif

			lFib = strtol(pcDup, &pcEnd, 0);
			if ((char *)0 != pcEnd && '\000' != *pcEnd) {
				fprintf(stderr, "%s: %s: %s:%d: fib: \"%s\" badly formatted counting numbner\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcDup);
				iRet = EX_DATAERR;
			}

#if HAVE_SETFIB
			iMibLen = sizeof(aiMib)/sizeof(aiMib[0]);
			iDataLen = sizeof(iMaxFib);
			if (-1 == sysctlnametomib(acNetFibs, aiMib, &iMibLen)) {
				fprintf(stderr, "%s: %s: %s:%d: fib: %s: no such mib?\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, acNetFibs);
			} else if (-1 == sysctl(aiMib, iMibLen, &iMaxFib, &iDataLen, NULL, 0)) {
				fprintf(stderr, "%s: %s: %s:%d: fib: sysctl: %s\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, strerror(errno));
			}
#endif

			if (0 > lFib) {
				fprintf(stderr, "%s: %s: %s:%d: %ld: negative fib value\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, lFib);
				iRet = EX_PROTOCOL;
			} else if (iMaxFib <= lFib) {
				fprintf(stderr, "%s: %s: %s:%d: %ld: fib value out of range (limit %ld)\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, lFib, (long)iMaxFib);
				iRet = EX_PROTOCOL;
			}
		}
		if ((char *)0 != (pcDup = FindOpt(cmd, "nice"))) {
			register long lNice;

			lNice = strtol(pcDup, &pcEnd, 0);
			if ((char *)0 != pcEnd && '\000' != *pcEnd) {
				fprintf(stderr, "%s: %s: %s:%d: nice: \"%s\" badly formatted\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcDup);
				iRet = EX_DATAERR;
			}
			if (-20 > lNice || 20 < lNice) {
				fprintf(stderr, "%s: %s: %s:%d: %ld: nice value out of range\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, lNice);
				iRet = EX_PROTOCOL;
			}
		}
		if (!CheckRedir(cmd, "<", "stdin")) {
			iRet = EX_PROTOCOL;
		}
		if (!CheckRedir(cmd, ">", "stdout")) {
			iRet = EX_PROTOCOL;
		}
		if (!CheckRedir(cmd, ">", "stderr")) {
			iRet = EX_PROTOCOL;
		}
		if ((char *)0 != (pcDup = FindOpt(cmd, "umask"))) {
			register long lMask;

			lMask = strtol(pcDup, &pcEnd, 8);
			if ((char *)0 != pcEnd && '\000' != *pcEnd) {
				fprintf(stderr, "%s: %s: %s:%d: umask: \"%s\" badly formatted\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcDup);
				iRet = EX_DATAERR;
			}
			if (0 > lMask || 0777 < lMask) {
				fprintf(stderr, "%s: %s: %s:%d: umask %04lo: value out of range\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, lMask);
				iRet = EX_PROTOCOL;
			}
		}
		if ((char *)0 != (pcDup = FindOpt(cmd, "environment")) && '\000' != *pcDup && (char *)0 != (pcField = CheckCompile(pcDup, "environment"))) {
			fprintf(stderr, "%s: %s: %s:%d: environment: \"%s\"\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcField);
			iRet = EX_DATAERR;
		}
		if ((char *)0 != (pcRead = FindOpt(cmd, acOptPam)) && '\000' != *pcRead && 0 != strcmp(acMyself, pcRead)) {
			if ((char *)0 != strchr(pcRead, ',')) {
				fprintf(stderr, "%s: %s: %s:%d: pam %s: specify a single application name or %s, not a list\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcRead, acMyself);
				iRet = EX_CONFIG;
			} else if (LooksLikeRE(pcRead)) {
				fprintf(stderr, "%s: %s: %s:%d: pam %s: specify a single application name or %s, not a regular expression\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcRead, acMyself);
				iRet = EX_CONFIG;
#if !USE_PAM
			} else {
				fprintf(stderr, "%s: %s: %s:%d: pam unsupported\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcRead, acMyself);
				iRet = EX_UNAVAILABLE;
#endif
			}
		}
		if ((char *)0 != (pcRead = FindOpt(cmd, acOptPass))) {
			iMatch = 0;
			if ('\000' == *pcRead) {
				pcRead = acMyself;
			}
			while ((char *)0 != (pcRead = GetField(pcRead, &pcField))) {
				if ((0 == strcmp("%u", pcField) || 0 == strcmp(acMyself, pcField)) && 0 != strcmp(pcRead, "%f")) {
					iMatch = 1;
					continue;
				}
				if ((struct passwd *)0 != getpwnam(pcField)) {
					iMatch = 1;
				}
			}
			if (0 == iMatch) {
				fprintf(stderr, "%s: %s: %s:%d: no passwords available from logins in list\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline);
				iRet = EX_NOUSER;
			}
		}
		for (iIndex = 0; iIndex < cmd->nopts; ++iIndex) {
			register const char *pcScan, *pcAttr;
			register int iLen, i;

			pcRead = cmd->opts[iIndex];
			if ((char *)0 == (pcScan = strchr(pcRead, '='))) {
				iLen = strlen(pcRead);
			} else {
				iLen = pcScan-pcRead;
			}

			/* Just ignore $ENVs, $# should be a number (above),
			 * compile $1,$2, etc and $*, all $N <= lHelpParam,
			 * fall into !g, !u, !f checks (they match isalpha).
			 */
			if ('$' == pcRead[0] && (isalpha(pcRead[1]) || '_' == pcRead[1])) {
				continue;
			} else if ('%' == pcRead[0]) {
				/* see next */
			} else if ('!' == pcRead[0] && isalpha(pcRead[1])) {
				/* see the next stanza */
			} else if ('#' == pcRead[1]) {
				/* did above */
				continue;
			} else if ('*' == pcRead[1] || isdigit(pcRead[1])) {
				if (isdigit(pcRead[1]) && -1 != Need.lHelpParam) {
					lDup = strtol(pcRead+1, &pcEnd, 0);
					if ('!' == pcRead[0] && '\000' == pcEnd[0]) {
						if (Need.lHelpParam >= lDup) {
							fprintf(stderr, "%s: %s: %s:%d: %.*s: forced limit of %ld requires more parameters than we allow\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, iLen, pcRead, Need.lHelpParam);
						}
						continue;
					}
					if (lDup > Need.lHelpParam) {
						fprintf(stderr, "%s: %s: %s:%d: %.*s: parameter too large to match forced limit of %ld\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, iLen, pcRead, Need.lHelpParam);
						iRet = EX_UNAVAILABLE;
					}
				}
				if ('0' == pcRead[1]) {
					fprintf(stderr, "%s: %s: %s:%d: %.*s: mis-leading '0':positional parameters are not numbered in octal\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, iLen, pcRead);
					iRet = EX_CANTCREAT;
				}
				if ((const char *)0 == pcScan) {
					continue;
				}
				if ((char *)0 != (pcField = CheckCompile(pcScan+1, pcRead))) {
					fprintf(stderr, "%s: %s: %s:%d: %.*s: \"%s\"\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, iLen, pcRead, pcField);
					iRet = EX_DATAERR;
				}
				continue;
			} /* $<trash> in the environment is ok by me */

			if (!('!' == pcRead[0] || '%' == pcRead[0])) {
				/* nada */
			} else if ('g' == pcRead[1]) {
				fSpecG = 1;
				switch (iLen) {
				case 2: /* %g, !g */
					break;
				case 4: /* %g@u, !g@u */
					if ('@' != pcRead[2] || 'u' != pcRead[3]) {
						/*FALLTHROUGH*/
				default:
						fprintf(stderr, "%s: %s: %s:%d: %%u unknown qaulifier \"%.*s\"\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, iLen-2, pcRead+2);
						iRet = EX_USAGE;
						break;
					}
					break;
				}
				if ((char *)0 != (pcField = CheckCompile(pcScan+1, pcRead))) {
					fprintf(stderr, "%s: %s: %s:%d: %.2s: \"%s\"\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, pcRead, pcField);
					iRet = EX_DATAERR;
				}
				if (! Need.fGid) {
					fprintf(stderr, "%s: %s: %s:%d: %.*s: specification never used\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, iLen, pcRead);
					/* not worth a non-zero exit code */
				}
				continue;
			} else if ('u' == pcRead[1]) {
				fSpecU = 1;
				switch (iLen) {
				case 2: /* %u, !u */
					break;
				case 4: /* %u@g, !u@g */
					if ('@' != pcRead[2] || 'g' != pcRead[3]) {
						/*FALLTHROUGH*/
				default:
						fprintf(stderr, "%s: %s: %s:%d: %%u unknown qaulifier \"%.*s\"\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, iLen-2, pcRead+2);
						iRet = EX_USAGE;
						break;
					}
					break;
				}
				if ((char *)0 != (pcField = CheckCompile(pcScan+1, pcRead))) {
					fprintf(stderr, "%s: %s: %s:%d: %.*s: \"%s\"\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, iLen, pcRead, pcField);
					iRet = EX_DATAERR;
				}
				if (! Need.fUid) {
					fprintf(stderr, "%s: %s: %s:%d: %.*s: specification never used\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, iLen, pcRead);
					/* not worth a non-zero exit code */
				}
				continue;
			} else if ('f' == pcRead[1] && '.' == pcRead[2]) {
				fSpecF = 1;
				if (! Need.fSecFile) {
					fprintf(stderr, "%s: %s: %s:%d %%f: specification never used\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline);
					/* not worth a non-zero exit code */
					Need.fSecFile = 1;
				}
				for (pcAttr = asFKnown; '\000' != *pcAttr; pcAttr += strlen(pcAttr)+1) {
					if (0 == strncmp(pcAttr, pcRead+3, iLen-3))
						break;
				}
				if ('\000' == *pcAttr) {
					fprintf(stderr, "%s: %s: %s:%d: %%f: attribute \"%.*s\"\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, iLen-3, pcRead+3);
					iRet = EX_USAGE;
				} else if ((char *)0 != (pcField = CheckCompile(pcScan+1, pcRead))) {
					fprintf(stderr, "%s: %s: %s:%d: %.*s: \"%s\"\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, iLen, pcRead, pcField);
					iRet = EX_DATAERR;
				}
				continue;
			}
			for (i = 0; (char *)0 != (pcScan = apcConfHelp[i]); ++i) {
				if (0 == strncmp(pcRead, pcScan, iLen))
					break;
			}
			if ((char *)0 == pcScan) {
				fprintf(stderr, "%s: %s: %s:%d: unknown keyword \"%.*s\"\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline, iLen, pcRead);
				iRet = EX_USAGE;
			}
		}
		if (acDefault == cmd->pcname) {
			continue;
		}
		if (!fSpecU && Need.fUid) {
			fprintf(stderr, "%s: %s: %s:%d: %%u: is unlimited\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline);
			iRet = EX_NOPERM;
		}
		if (!fSpecG && Need.fGid) {
			fprintf(stderr, "%s: %s: %s:%d: %%g: is unlimited\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline);
			iRet = EX_NOPERM;
		}
		if (!fSpecF && Need.fSecFile) {
			fprintf(stderr, "%s: %s: %s:%d: %%f: is unlimited\n", Progname, cmd->pcname, cmd->pcfile, cmd->iline);
			iRet = EX_NOPERM;
		}
	}

	if ((cmd_t *)0 == cmd) {
		fprintf(stderr, "%s: no rules configured%s\n", Progname, acSure);
	}
	}

	exit(iRet);
}

/* Given the complete RE-list for a parameter make a usage string.	(ksb)
 * If we build the string you can free it, if we return pcDef you must
 * know what to do with it.  Since the return from all the fields must
 * be at most the size of all the fields minus the anchors we know it must
 * fit in a buffer as long as the input.  No other overrun checks needed.
 */
static const char *
FieldToUsage(const char *pcREList, const char *pcDef)
{
	register char *pcRet;
	register const char *pcCheck;
	auto char *pcField;

	/* 2* for the "number" case of "^5$" as a field
	 */
	if ((char *)0 == (pcRet = malloc((2*strlen(pcREList)|15)+1))) {
		return pcDef;
	}
	*pcRet = '\000';
	while ((char *)0 != (pcREList = GetField(pcREList, &pcField))) {
		pcCheck = IsAnchored(pcField, (const char *)0);
		if ((char *)0 == pcCheck) {
			free((void *)pcRet);
			return pcDef;
		}
		if ('\000' != *pcRet) {
			(void)strcat(pcRet, "|");
		}
		(void)strcat(pcRet, pcCheck);
	}
	return pcRet;
}

/* We split the difference by default we'll show you the rules, but you
 * can remove it with a -DOP_SHOW_RULES=0
 */
#if !defined(OP_SHOW_RULES)
#define OP_SHOW_RULES	1
#endif

/* Output $0 [-g group] [-u user] [-f file] mnemonic...			(ksb)
 * We drop for this, as we shouldn't have to do anything super-ish.
 * We can view the used (-l), or the run-time (-r) or that plus why (-w).
 */
static void
RulesAllowed(int fWhichView, int argc, char **argv)
{
#if OP_SHOW_RULES
	register cmd_t *cmd, *cmdScan;
	register long lScan;
	register const char *pcRead, *pcCodeCheck, *pcLook;
	register size_t i;
	auto const char *pcUserMay;
	auto int uOrig, fTold, iExit;
	auto struct passwd *pw;
	auto struct group *gr;
	auto char *pcField;
	auto char acCvt[OP_MAX_LDWIDTH];	/* "$%ld" or "#%ld" */
	auto char acMyName[512];
	auto NEEDS Need;

	iExit = EX_OK;
	uOrig = getuid();
	switch (argc) {
	case 0:
		if ((struct passwd *)0 == (pw = getpwuid(uOrig)))
			fatal("getpwuid: %ld: %s", (long)uOrig, strerror(errno));
		break;
	case 1:
		if (0 != uOrig)
			fatal(acBadAllow);
		if ((struct passwd *)0 == (pw = getpwnam(argv[0])))
			fatal("getpwnam: %s: %s", argv[0], strerror(errno));
		if (0 != initgroups(argv[0], pw->pw_gid))
			fatal("initgroups: %s", strerror(errno));
		if (0 != setgid(pw->pw_uid))
			fatal("setgid: %ld: %s", (long)pw->pw_uid, strerror(errno));
		uOrig = pw->pw_uid;
		break;
	default:
		fatal("usage: -l login");
	}
	if (setreuid(uOrig, uOrig) < 0) {
		fatal("setuid: %ld: %s", uOrig, strerror(errno));
	}
	if (-1 == gethostname(acMyName, sizeof(acMyName))) {
		fatal("gethostname: %s", strerror(errno));
	}

	fTold = 0;
	for (cmdScan = NewCmd((const char *)0); (cmd_t *)0 != cmdScan; cmdScan = cmdScan->pODnext) {
		if (acDefault == cmdScan->pcname)
			continue;
		if ((cmd_t *)0 == (cmd = Build(cmdScan)))
			continue;

		/* Can the Customer run it?  Allow root to see them all,
		 * but flag ones she can't run (with a hash comment).
		 */
		pcUserMay = (const char *)0;
		if ((const char *)0 == pcUserMay && (char *)0 != (pcRead = FindOpt(cmd, acOptGroups))) {
			if ((struct group *)0 == (gr = getgrgid(pw->pw_gid))) {
				if (AnyMatch(pcRead, ":", pw->pw_gid, acOptGroups)) {

					snprintf(acCvt, sizeof(acCvt), "#%ld", (long)pw->pw_gid);
					pcUserMay = "login group gid";
				}
			} else if (AnyMatch(pcRead, gr->gr_name, gr->gr_gid, acOptGroups)) {
				pcUserMay = "login group name";
			}
			setgrent();
			while ((const char *)0 == pcUserMay && (struct group *)0 != (gr = getgrent())) {
				for (i = 0; (char *)0 != gr->gr_mem[i]; ++i) {
					if (0 == strcmp(gr->gr_mem[i], pw->pw_name))
						break;
				}
				if (((char *)0 != gr->gr_mem[i]) && AnyMatch(pcRead, gr->gr_name, gr->gr_gid, acOptGroups)) {
					pcUserMay = "group membership";
					break;
				}
			}
		}
		if ((const char *)0 == pcUserMay && ((char *)0 != (pcRead = FindOpt(cmd, acOptUsers))) && AnyMatch(pcRead, pw->pw_name, pw->pw_uid, acOptUsers)) {
			pcUserMay = (0 == strcmp(pcRead, "^.*$") || 0 == strcmp(pcRead, ".*")) ? "anyone" : "login";
		}
		if ((const char *)0 == pcUserMay && ((char *)0 != (pcRead = FindOpt(cmd, acOptNetgroups)))) {
			while ((char *)0 != (pcRead = GetField(pcRead, &pcField))) {
				if (innetgr(pcField, acMyName, pw->pw_name, (char *)0)) {
					pcUserMay = "netgroup membership";
					break;
				}
			}
		}

		if ((const char *)0 == pcUserMay && 0 != uOrig) {
			continue;
		}

		/* if craving fails it is a format error in the params
		 */
		if (0 != Craving(cmd, & Need)) {
			if (!fTold) {
				fTold = 1;
				fprintf(stderr, "%s: Please ask the administrator to run -S and fix our configuration.\n", Progname);
			}
			iExit = EX_CONFIG;
			printf("#");
		} else if (0 == uOrig && (const char *)0 == pcUserMay) {
			printf("#");
		}

		/* under -r or -w list the command we are talking about
		 */
		if ('r' == fWhichView || 'w' == fWhichView) {
			register long lCur;

			if ((const char *)0 == (pcCodeCheck = FindOpt(cmd, "jacket"))) {
				pcCodeCheck = FindOpt(cmd, "helmet");
			}
			printf("%s", cmd->pcname);
			for (lScan = 0; lScan < cmd->nargs; ++lScan) {
				pcRead = pcLook = cmd->args[lScan];
				if ('$' != pcLook[0]) {
					pcRead = pcLook;
				} else if (isdigit(pcLook[1]) && (lCur = strtol(pcLook+1, & pcField, 0), '\000' == *pcField) && (char *)0 != (pcRead = FindOpt(cmd, pcLook))) {
					pcRead = FieldToUsage(pcRead, pcLook);
				} else if ('\000' == pcLook[1] || '\000' != pcLook[2]) {
					/* as is */
				} else switch (pcLook[1]) {
				case '0':
					printf(" %s", cmd->pcname);
					continue;
				case '@':
				case '*':
					printf(" args");
					continue;
				case 's':
					printf(" {script}");
					continue;
				case 'S':
					printf(" $SHELL");
					continue;
				case '$':
					printf(" $");
					continue;
				case '|':	/* a forced empty word */
					printf(" ''");
				default:
					break;
				}
				if ((char *)0 == pcRead) {
					printf(" %s", pcLook);
					continue;
				}
				printf(" %s", pcRead);
				if (pcLook != pcRead) {
					free((void *)pcRead);
				}
			}
			if ((char *)0 != (pcField = FindOpt(cmd, acOptDaemon))) {
				pcField = (char *)acDevNull;
			}
			if ((char *)0 != (pcRead = FindOpt(cmd, "stdin")) || (char *)0 != (pcRead = pcField)) {
				printf(" ");
				if ('<' != *pcRead && '>' != *pcRead)
					printf("<");
				printf("%s", pcRead);
			}
			if ((char *)0 != (pcRead = FindOpt(cmd, "stdout")) || (char *)0 != (pcRead = pcField)) {
				printf(" ");
				if ('<' != *pcRead && '>' != *pcRead)
					printf(">");
				printf("%s", pcRead);
			}
			if ((char *)0 != (pcRead = FindOpt(cmd, "stderr")) || (char *)0 != (pcRead = pcField)) {
				printf(" 2");
				if ('<' != *pcRead && '>' != *pcRead)
					printf(">");
				printf("%s", pcRead);
			}
			if ((char *)0 != pcField) {
				printf(" &");
			}
			if ('w' == fWhichView) {
				register const char *pcUserAuth;

				if ((const char *)0 == pcUserMay) {
					printf(" [may not");
				} else if ((const char *)0 != pcCodeCheck) {
					printf(" [%s plus %s", pcUserMay, pcCodeCheck);
				} else {
					printf(" [by %s", pcUserMay);
				}
				pcRead = FindOpt(cmd, acOptPam);
				if ((char *)0 != pcRead && '\000' == *pcRead) {
					pcRead = (const char *)0;
				}
				if ((char *)0 == FindOpt(cmd, acOptPass)) {
					pcUserAuth = ((char *)0 == pcRead) ? (const char *)0 : "pam";
				} else {
					pcUserAuth = ((char *)0 == pcRead) ? "password" : "either pam or password";
				}
				if ((const char *)0 != pcUserAuth && (const char *)0 != pcUserMay) {
					printf(" with %s authentication", pcUserAuth);
				}
				printf("]");
			}
			printf("\n");
			continue;
		}

		/* under -l list the command-line form
		 */
		printf("%s", Progname);
		if (Need.fUid)
			printf(" -u uid");
		if (Need.fGid)
			printf(" -g gid");
		if (Need.fSecFile)
			printf(" -f file");
		printf(" %s", cmd->pcname);
		for (lScan = 1; lScan <= Need.lHelpParam; ++lScan) {
			snprintf(acCvt, sizeof(acCvt), "$%ld", lScan);
			if ((char *)0 == (pcRead = FindOpt(cmd, acCvt))) {
				printf(" %s", acCvt);
				continue;
			}

			/* If the pattern is looks like a usage "^(a|b|c)$"
			 * or "^word$" then we'll output it filtered, else
			 * just output $n.
			 */
			pcRead = FieldToUsage(pcRead, acCvt);
			printf(" %s", pcRead);
			if (acCvt != pcRead) {
				free((void *)pcRead);
			}
		}

		/* If we have a $* figure out if it is plural:
		 * viz. when no total param set, also when no spec. max and
		 * total param > 1, else when max is less than the forced total
		 */
		if (Need.fDStar && (-1 == Need.lTotalParam || Need.lHelpParam < Need.lTotalParam)) {
			static const char acS[] = "s";
			register const char *pcPlural = acS+1;

			if (-1 == Need.lTotalParam) {
				pcPlural = acS;
			} else if (0 == Need.lCmdParam) {
				if (1 != Need.lTotalParam)
					pcPlural = acS;
			} else if (Need.lCmdParam < Need.lTotalParam) {
				pcPlural = acS;
			}
			printf(" [arg%s]", pcPlural);
		}
		printf("\n");
	}
	exit(iExit);
#else
	fatal("Sorry, that option is forbidden by site policy");
	exit(EX_UNAVAILABLE);
#endif
}

#if USE_PAM
/* call-back to us from pam_* functions for pam option			(ksb)
 * Another Sun compiler botch: because I mark this static to limit the
 * scope the compiler tells me the type is different in the assignment to
 * any (struct pam_conv).conv, storage class is not a type, bozos.
 */
static int
OpConv(int iMsg, const struct pam_message **ppmsg, struct pam_response **ppresp, void *pvAppData)
{
	register int i;
	register char *pcFetch;
	auto size_t wLen;

	if ((struct pam_response *)0 == (*ppresp = calloc(iMsg, sizeof(struct pam_response)))) {
		fatal(acBadAlloc);
	}
	(void)pvAppData;	/* the cmd structure we don't need (yet) */
	for (i = 0; i < iMsg; ++i) {
		ppresp[i]->resp = (char *)0;
		ppresp[i]->resp_retcode = 0;
		switch (ppmsg[i]->msg_style) {
		case PAM_PROMPT_ECHO_ON:
			if ((char *)0 == (pcFetch = fgetln(stdin, & wLen)))
				fatal("fgetln: %s", strerror(errno));
			if (wLen > 0 && '\n' == pcFetch[wLen-1])
				--wLen, pcFetch[wLen] = '\000';
			break;
		case PAM_PROMPT_ECHO_OFF:
			if ((char *)0 == (pcFetch = getpass("Password:")))
				fatal("getpass: %s", strerror(errno));
			wLen = strlen(pcFetch);
			break;
		case PAM_TEXT_INFO:
			if ((char *)0 != ppmsg[i]->msg)
				puts(ppmsg[i]->msg);
			continue;
		case PAM_ERROR_MSG:
			if ((char *)0 != ppmsg[i]->msg)
				fprintf(stderr, "%s\n", ppmsg[i]->msg);
			continue;
		default:
			fatal("pam sent a message type (%d)", ppmsg[i]->msg_style);
			return PAM_CONV_ERR;
		}
		if ((char *)0 == (ppresp[i]->resp = calloc((wLen|15)+1, sizeof(char))))
			fatal(acBadAlloc);
		(void)strncpy(ppresp[i]->resp, pcFetch, wLen+1);
	}
	return PAM_SUCCESS;
}
#endif

/* Provide a dynamic copy of the reason this access is being granted,	(ksb)
 * viz.	cred:value\000
 */
static char *
KeepMay(const char *pcCred, const char *pcValue)
{
	register size_t iLen;
	register char *pcRet;

	iLen = ((strlen(pcCred)+1+strlen(pcValue))|15)+1;
	if ((char *)0 == (pcRet = malloc(iLen)))
		fatal(acBadAlloc);
	snprintf(pcRet, iLen, "%s:%s", pcCred, pcValue);
	return pcRet;
}

/* Find the mnemonic based on the command-line arguments given.		(ksb)
 * When both pam and password are set, must pass one of them (not both).
 */
static const char *
Verify(const cmd_t *cmd, const int argc, const char **argv, const char **ppcFail)
{
	register int iParam, i;
	register const char *pcPass, *pcScan, *pcAskPass, *pcPam;
	register struct passwd *pw;
	register struct group *gr;
#if USE_GETSPNAM
	register struct spwd *spw;
#endif
	auto char *pcField, *pcRest;
	auto const char *pcUserMay;
	static char acDummy[] = "=.";

	pcUserMay = (const char *)0;
	*ppcFail = acBadAllow;
	pcAskPass = FindOpt(cmd, acOptPass);
	if ((const char *)0 == (pcPam = FindOpt(cmd, acOptPam))) {
		/* nada */
	} else if ('\000' == *pcPam) {
		/* allow a reset to no pam, when DEFAULT is pam auth */
		pcPam = (const char *)0;
	} else {
#if USE_PAM
		static char acPromptTempl[] = "%s credentials:";
		auto struct pam_conv local_conv;
		auto int iStatus, iMem;
		auto pam_handle_t *pPCLocal;
		register char *pcPrompt;

		local_conv.appdata_ptr = (void *)cmd;
		local_conv.conv = OpConv;
		if ('.' == pcPam[0] && '\000' == pcPam[1]) {
			pcPam = acDefPam;
		}
		if (PAM_SUCCESS != (iStatus = pam_start(pcPam, pcPerp, &local_conv, &pPCLocal))) {
			fatal("pam_start: %s: %s", pcPam, pam_strerror(pPCLocal, errno));
		}
		(void)pam_set_item(pPCLocal, PAM_TTY, acDevTty);
		(void)pam_set_item(pPCLocal, PAM_RHOST, "localhost");
		(void)pam_set_item(pPCLocal, PAM_RUSER, pcPerp);
		iMem = (sizeof(acPromptTempl)+strlen(pcPam))|15;
		if ((char *)0 == (pcPrompt = calloc(iMem+1, sizeof(char))))
			fatal(acBadAlloc);
		snprintf(pcPrompt, iMem, acPromptTempl, pcPam);
		(void)pam_set_item(pPCLocal, PAM_USER_PROMPT, pcPrompt);

		switch (iStatus = pam_authenticate(pPCLocal, 0)) {
		case PAM_SUCCESS:
			/* pam likes us, don't ask for a password either
			 */
			pcPam = pcAskPass = (const char *)0;
			break;
		case PAM_AUTH_ERR:
		case PAM_CONV_ERR:
		case PAM_CRED_INSUFFICIENT:
		case PAM_MAXTRIES:
		case PAM_PERM_DENIED:
			/* check password below */
			break;
		default:
		/* PAM_{AUTHINFO_UNAVAIL,ABORT,BUF_ERR,SERVICE_ERR}
		 * PAM_{SYMBOL_ERR,SYSTEM_ERR,USER_UNKNOWN}:
		 */
			fatal("pam: %s: %s", pcPam, pam_strerror(pPCLocal, iStatus));
			/*NOTREACHED*/
		}
		if (PAM_SUCCESS != (iStatus = pam_end(pPCLocal, iStatus))) {
			fatal("pam_end: %s: %s", pcPam, pam_strerror(pPCLocal, iStatus));
		}
#else
		fprintf(stderr, "%s: %s: %s: no support on this host\n", Progname, acOptPam, pcPam);
#endif
	}

	/* Must know a password on one of these logins, default to ours
	 */
	if ((const char *)0 != pcAskPass) {
		register const char *pcIttr;
		auto struct stat stPass;
		extern char *crypt();

		if ((char *)0 == (pcPass = getpass("Password:"))) {
			return "No password available";
		}
		if ('\000' == *pcAskPass) {
			pcAskPass = acMyself;
		}
		pcScan = pcAskPass;
		while ((char *)0 != (pcScan = GetField(pcScan, &pcField))) {
			if (0 == strcmp(pcField, acMyself)) {
				pcIttr = pcPerp;
			} else if (0 != strcmp(pcField, "%f")) {
				if ((char *)0 == pcSecFile) {
					*ppcFail = "No -f specified";
					continue;
				}
				if (-1 == stat(pcSecFile, &stPass)) {
					*ppcFail = "Cannot stat -f target for password lookup";
					continue;
				}
				if ((struct passwd *)0 == (pw = getpwuid(stPass.st_uid))) {
					*ppcFail = "-f owner unmapped for password check";
					continue;
				}
				/* small memory leak here */
				pcIttr = strdup(pw->pw_name);
			} else if (0 != strcmp(pcField, "%u")) {
				pcIttr = pcScan;
			} else if ((char *)0 == (pcIttr = pcUid)) {
				*ppcFail = "No -u specified";
				continue;
			}
#if USE_GETSPNAM
			if ((struct spwd *)0 == (spw = getspnam(pcIttr)))
				continue;
#else
			if ((struct passwd *)0 == (pw = getpwnam(pcIttr)))
				continue;
#endif
#if USE_GETSPNAM
			if (0 != strcmp(crypt(pcPass, spw->sp_pwdp), spw->sp_pwdp))
				continue;
#else
			if (0 != strcmp(crypt(pcPass, pw->pw_passwd), pw->pw_passwd))
				continue;
#endif
			break;
		}
		if ((char *)0 == pcScan) {
			return (const char *)0;
		}
		pcPam = (const char *)0;
	}
	if ((char *)0 != pcPam) {
		return (const char *)0;
	}

	/* I don't think we ever should allow running as an unmapped uid.
	 * But if your primary login group is unmapped we'll call it ":",
	 * which can't really be a group, but you could match in a rule.
	 * Why you'd want a rule to allow an unmapped primary group, I don't
	 * really know.  Be liberal with what we allow in the configuration.
	 */
	if ((struct passwd *)0 == (pw = getpwuid(getuid()))) {
		return (const char *)0;
	}

	if ((const char *)0 == pcUserMay && (char *)0 != (pcScan = FindOpt(cmd, acOptGroups))) {
		auto char acCvt[OP_MAX_LDWIDTH];	/* "#%ld" */

		if ((struct group *)0 == (gr = getgrgid(pw->pw_gid))) {
			if (AnyMatch(pcScan, ":", pw->pw_gid, acOptGroups)) {

				snprintf(acCvt, sizeof(acCvt), "#%ld", (long)pw->pw_gid);
				pcUserMay = KeepMay(acOptGroups, acCvt);
			}
		} else if (AnyMatch(pcScan, gr->gr_name, gr->gr_gid, acOptGroups)) {
			pcUserMay = KeepMay(acOptGroups, gr->gr_name);
		}
		setgrent();
		while ((const char *)0 == pcUserMay && (struct group *)0 != (gr = getgrent())) {
			for (i = 0; (char *)0 != gr->gr_mem[i]; ++i) {
				if (0 == strcmp(gr->gr_mem[i], pw->pw_name))
					break;
			}
			if (((char *)0 != gr->gr_mem[i]) && AnyMatch(pcScan, gr->gr_name, gr->gr_gid, acOptGroups)) {
				pcUserMay = KeepMay(acOptGroups, gr->gr_name);
				break;
			}
		}
	}
	if ((const char *)0 == pcUserMay && ((char *)0 != (pcScan = FindOpt(cmd, acOptUsers))) && AnyMatch(pcScan, pw->pw_name, pw->pw_uid, acOptUsers)) {
		pcUserMay = KeepMay(acOptUsers, pw->pw_name);
	}
	if ((const char *)0 == pcUserMay && ((char *)0 != (pcScan = FindOpt(cmd, acOptNetgroups)))) {
		auto char acMyName[512];

		if (-1 == gethostname(acMyName, sizeof(acMyName))) {
			fatal("gethostname: %s", strerror(errno));
		}
		while ((char *)0 != (pcScan = GetField(pcScan, &pcField))) {
			if (innetgr(pcField, acMyName, pw->pw_name, (char *)0)) {
				pcUserMay = KeepMay(acOptNetgroups, pcField);
				break;
			}
		}
	}

	/* One might think that the superuser (real uid) might be able
	 * to do anything -- they don't need op to do that!
	 */
	if ((const char *)0 == pcUserMay) {
		return (const char *)0;
	}
	*ppcFail = acOK;

	/* Look for $*, $[0-9]*, and repeat the check from Find.
	 * $5 w/o an R.E. means must be non-empty (default RE of /./)
	 */
	for (i = 0; i < cmd->nopts; ++i) {
		pcScan = cmd->opts[i];
		if ('$' != pcScan[0] && '!' != pcScan[0]) {
			continue;
		}
		/* look for $*=REs
		 */
		if ('*' == pcScan[1]) {
			/* must check in Go */
			continue;
		}
		if (!isdigit(cmd->opts[i][1])) {
			continue;
		}
		/* Found $[0-9]+=REs, $0 is configuration error, dude.
		 * I thought about using it... too unclear (match our name)
		 * "$5" is the same as "$5=." (has at least 1 character)
		 * "!5" is the same as $# < 5
		 */
		if (0 == (iParam = strtoul(pcScan+1, & pcRest, 10))) {
			return acBadConfig;
		}
		if ('\000' == *pcRest) {
			/* set the deault and allow the termination code
			 * to write over the '=' above, as if to end the
			 * number in "$7=." for the tag
			 */
			pcRest = acDummy+1;
		} else if ('=' != *pcRest++) {
			return acBadConfig;
		}
		if (iParam >= argc) {
			if ('!' == pcScan[0] && acDummy+1 == pcRest)
				continue;
			return acBadFew;
		}
		pcRest[-1] = '\000';
		if (! GenMatch(pcScan, argv[iParam], pcRest)) {
			pcRest[-1] = '=';
			if ('!' == pcScan[0])
				continue;
			return acBadParam;
		}
		pcRest[-1] = '=';
		if ('!' == pcScan[0]) {
			return acBadParam;
		}
	}
	return pcUserMay;
}

/* Append the tail onto the dir to make an absolute path.		(ksb)
 * I wish every OS had realpath.  Sigh.
 * Inv.: pcDir points to at least MAXPATHLEN characters.
 */
static char *
FullPath(char *pcDir, const char *pcTail)
{
	register int iStrip, iLen, c;
	register char *pcSnip, *pcLast;
	auto char acLink[MAXPATHLEN+4];

	if ('\000' == *pcTail) {
		return pcDir;
	}
	for (iStrip = 0; '.' == pcTail[0]; /* nada */) {
		if ('.' == pcTail[1] && ('\000' == pcTail[2] || '/' == pcTail[2]))
			++iStrip, pcTail += 2;
		else if ('/' == pcTail[1] || '\000' == pcTail[1])
			pcTail += 1;
		else
			break;
		while ('/' == *pcTail)
			++pcTail;
	}
	while (iStrip-- > 0) {
		if ('\000' == pcDir[1])
			break;
		if ((char *)0 == (pcSnip = strrchr(pcDir, '/')))
			break;
		while ('/' == *pcSnip && pcDir != pcSnip)
			*pcSnip-- = '\000';
		if (pcDir == pcSnip)
			pcDir[1] = '\000';
	}
	iLen = strlen(pcDir);
	if (iLen+2 >= MAXPATHLEN) {
		/* out of space */
		return (char *)0;
	}
	pcLast = pcDir+iLen;
	if ('/' != pcDir[iLen-1] && '\000' != *pcTail) {
		pcDir[iLen++] = '/';
	}
	while ('\000' != (c = *pcTail++)) {
		if ('/' != c) {
			pcDir[iLen++] = c;
			if (iLen >= MAXPATHLEN) {
				/* out of space */
				return (char *)0;
			}
			continue;
		}
		pcDir[iLen] = '\000';
		while ('/' == *pcTail)
			++pcTail;
		if (-1 != (iLen = readlink(pcDir, acLink, sizeof(acLink)-1))) {
			*pcLast = '\000';
			acLink[iLen] = '\000';
			if ('/' == acLink[0]) {
				pcDir[0] = '/';
				pcDir[1] = '\000';
				for (pcSnip = acLink; '/' == *pcSnip; ++pcSnip)
					/* nada */;
				pcDir = FullPath(pcDir, pcSnip);
			} else {
				pcDir = FullPath(pcDir, acLink);
			}
			if ((char *)0 == pcDir) {
				return (char *)0;
			}
		}
		return FullPath(pcDir, pcTail);
	}
	pcDir[iLen] = '\000';
	return pcDir;
}

/* Convert a file mode into a type character.				(ksb)
 *	[-cbdpsl] like common ls,
 *	n for a non-existant file,
 *	D doors from Solaris,
 *	P ports from Solaris/Xenix,
 *	w white-outs,
 * I removed contiguous file support as I've not seen it in decades
 */
static char
FileType(const struct stat *pstThis)
{
	if ((struct stat *)0 == pstThis)
		return 'n';
#if defined(S_IFIFO)
	if (S_IFIFO == (pstThis->st_mode&S_IFMT))
		return 'p';
#endif
#if defined(S_IFDOOR)
	if (S_IFDOOR == (pstThis->st_mode&S_IFMT))
		return 'D';
#endif
#if defined(S_IFSOCK)
	if (S_IFSOCK == (pstThis->st_mode&S_IFMT))
		return 's';
#endif
#if defined(S_IFBLK)
	if (S_IFBLK == (pstThis->st_mode&S_IFMT))
		return 'b';
#endif
#if defined(S_IFCHR)
	if (S_IFCHR == (pstThis->st_mode&S_IFMT))
		return 'c';
#endif
#if defined(S_IFREG)
	if (S_IFREG == (pstThis->st_mode&S_IFMT))
		return '-';
#endif
#if defined(S_IFDIR)
	if (S_IFDIR == (pstThis->st_mode&S_IFMT))
		return 'd';
#endif
#if defined(S_IFLNK)
	if (S_IFLNK == (pstThis->st_mode&S_IFMT))
		return 'l';
#endif
#if defined(S_IFWHT)
	if (S_IFWHT == (pstThis->st_mode&S_IFMT))
		return 'w';
#endif
#if defined(S_IFPORT)
	if (S_IFPORT == (pstThis->st_mode&S_IFMT))
		return 'P';
#endif
	return '-';
}

/* Is the given directory a mount-point?				(ksb)
 * In this case we know that the path is an absolute path to a dir, but we
 * are pretty general.  On some OS's st_ino == 2 is a tell, on others
 * it is not :-(.  We check ../ being a different st_dev, or being "/"
 * (even in a chroot op should think of slash as being a mount-point).
 * N.B.: We can write on pcPath, but we always put it back.
 * INV: strlen(pcPath) <= MAXPATHLEN
 */
static int
IsMountPt(const char *pcPath, const struct stat *pstPath)
{
	register char *pcTail;
	register int iKeep;
	auto char acAbove[MAXPATHLEN+8];	/* +"/..\000" */
	auto struct stat stAbove;

	if ((struct stat *)0 == pstPath) {
		return 0;
	}
	while ('/' == pcPath[0] && '/' == pcPath[1]) {
		++pcPath;
	}
	if ('/' == pcPath[0] && '\000' == pcPath[1]) {
		return 1;
	}

	if ((char *)0 == (pcTail = strrchr(pcPath, '/'))) {
		snprintf(acAbove, sizeof(acAbove), "%s/..", pcPath);
		if (-1 == stat(acAbove, &stAbove))
			return 0;
	} else if (pcTail == pcPath) {
		if (-1 == stat("/", &stAbove))
			return 0;
	} else {
		while (pcTail > pcPath && '/' == pcTail[-1])
			--pcTail;
		*pcTail = '\000';
		iKeep = stat(pcPath, &stAbove);
		*pcTail = '/';
		if (-1 == iKeep)
			return 0;
	}
	return stAbove.st_dev != pstPath->st_dev;
}

/* Return 1 if the directory is empty.					(ksb)
 */
static int
IsEmpty(const char *pcDir)
{
	register struct dirent *pDE;
	register DIR *pDR;
	register int iRet;

	if ((DIR *)0 == (pDR = opendir(pcDir))) {
		return 0;
	}
	iRet = 1;
	while ((struct dirent *)0 != (pDE = readdir(pDR))) {
		if ('.' != pDE->d_name[0])
			/* found one */;
		else if ('\000' == pDE->d_name[1])
			continue;
		else if ('.' != pDE->d_name[1])
			/* found a hidden file */;
		else if ('\000' == pDE->d_name[2])
			continue;
		iRet = 0;
		break;
	}
	closedir(pDR);
	return iRet;
}

/* Copied from install's special.c, which I wrote.			(ksb)
 */
static void
EBit(char *pcBit, const int bSet, const int cSet, const int cNotSet)
/*    pcBit pointer the the alpha bit			*/
/*     bSet should it set over struck			*/
/*     cSet overstrike with this			*/
/*  cNotSet it should have been this, else upper case	*/
{
	if (0 != bSet) {
		*pcBit = (cNotSet == *pcBit) ? cSet : toupper(cSet);
	}
}

/* Process the R.E. magic for a %f.$attr expression (or !f.$attr).	(ksb)
 * The largest text attribute a file can have is the path (MAXPATHLEN).
 * When you change asFKnown you have to add code here.
 */
static int
FileAttr(const char *pcFull, const struct stat *pstThis, const char *pcRE, const char *pcAttr)
{
	register struct passwd *pw;
	register struct group *gr;
	register int i, iBit;
	auto char acTarget[MAXPATHLEN+256];
	auto struct stat stIndir;

	if (0 == strcmp(pcAttr, "path")) {
		snprintf(acTarget, sizeof(acTarget), "%s", pcFull);
	} else if (0 == strcmp(pcAttr, "perms")) {
		/* copied from my install code -- the ls(1) symbolic mode
		 */
		(void)strcpy(acTarget, "-rwxrwxrwx");
		acTarget[0] = FileType(pstThis);
		for (i = 0, iBit = 0400; i < 9; ++i, iBit >>= 1) {
			if ((struct stat *)0 == pstThis || 0 == (pstThis->st_mode & iBit))
				acTarget[i+1] = '-';
		}
		/* Sun changes plain files group 'x' to 'l' -- but we don't
		 * it would just make it even harder to RE match -- ksb
		 */
		if ((struct stat *)0 != pstThis) {
			EBit(acTarget+3, S_ISUID & pstThis->st_mode, 's', 'x');
			EBit(acTarget+6, S_ISGID & pstThis->st_mode, 's', 'x');
			EBit(acTarget+9, S_ISVTX & pstThis->st_mode, 't', 'x');
		}
	} else if (0 == strcmp(pcAttr, "type")) {
		snprintf(acTarget, sizeof(acTarget), "%c", FileType(pstThis));
		if ('d' == acTarget[0] && IsMountPt(pcFull, pstThis)) {
			strncat(acTarget, "m", sizeof(acTarget));
		}
		if ('d' == acTarget[0] && IsEmpty(pcFull)) {
			strncat(acTarget, "e", sizeof(acTarget));
		}
		/* add the type of the other end on a 'l' (symbolic link)
		 */
		if ('l' == acTarget[0]) {
			if (-1 == stat(pcFull, &stIndir)) {
				acTarget[1] = FileType((struct stat *)0);
			} else {
				acTarget[1] = FileType(& stIndir);
			}
			acTarget[2] = '\000';
		}
	} else if (0 == strcmp(pcAttr, "access")) {
		(void)strcpy(acTarget, "rwxf");
		if (0 != access(pcFull, R_OK))
			acTarget[0] = '-';
		if (0 != access(pcFull, W_OK))
			acTarget[1] = '-';
		if ((struct stat *)0 == pstThis || 0 == (0111 & pstThis->st_mode) || 0 != access(pcFull, X_OK))
			acTarget[2] = '-';
		if (0 != access(pcFull, F_OK))
			acTarget[2] = '-';
	} else if ((struct stat *)0 == pstThis) {
		fatal("%s: %s", pcFull, strerror(ENOENT));
	} else if (0 == strcmp(pcAttr, "dev")) {
		snprintf(acTarget, sizeof(acTarget), "%ld", (long)pstThis->st_dev);
	} else if (0 == strcmp(pcAttr, "ino")) {
		snprintf(acTarget, sizeof(acTarget), "%ld", (long)pstThis->st_ino);
	} else if (0 == strcmp(pcAttr, "nlink")) {
		snprintf(acTarget, sizeof(acTarget), "%ld", (long)pstThis->st_nlink);
	} else if (0 == strcmp(pcAttr, "atime")) {
		snprintf(acTarget, sizeof(acTarget), "%ld", (long)pstThis->st_atime);
	} else if (0 == strcmp(pcAttr, "mtime")) {
		snprintf(acTarget, sizeof(acTarget), "%ld", (long)pstThis->st_mtime);
	} else if (0 == strcmp(pcAttr, "ctime")) {
		snprintf(acTarget, sizeof(acTarget), "%ld", (long)pstThis->st_ctime);
	} else if (0 == strcmp(pcAttr, "birthtime") || 0 == strcmp(pcAttr, "btime")) {
#if HAVE_BIRTHTIME
		snprintf(acTarget, sizeof(acTarget), "%ld", pstThis->st_birthtime);
#else
		snprintf(acTarget, sizeof(acTarget), "%ld", (long)pstThis->st_ctime);
#endif
	} else if (0 == strcmp(pcAttr, "size")) {
		snprintf(acTarget, sizeof(acTarget), "%ld", (long)pstThis->st_size);
	} else if (0 == strcmp(pcAttr, "blksize")) {
		snprintf(acTarget, sizeof(acTarget), "%ld", (long)pstThis->st_blksize);
	} else if (0 == strcmp(pcAttr, "blocks")) {
		snprintf(acTarget, sizeof(acTarget), "%ld", (long)pstThis->st_blocks);
	} else if (0 == strcmp(pcAttr, "uid")) {
		snprintf(acTarget, sizeof(acTarget), "%ld", (long)pstThis->st_uid);
	} else if (0 == strcmp(pcAttr, "login@g")) {
		if ((struct passwd *)0 == (pw = getpwuid(pstThis->st_uid)))
			fatal("getpwuid: %ld: no such login", (long)pstThis->st_uid);
		return MemberMatch(pcRE, pw->pw_name, & pw->pw_gid, "login@g");
	} else if (0 == strcmp(pcAttr, "login")) {
		if ((struct passwd *)0 == (pw = getpwuid(pstThis->st_uid)))
			fatal("getpwuid: %ld: no such login", (long)pstThis->st_uid);
		snprintf(acTarget, sizeof(acTarget), "%s", pw->pw_name);
	} else if (0 == strcmp(pcAttr, "gid")) {
		snprintf(acTarget, sizeof(acTarget), "%ld", (long)pstThis->st_gid);
	} else if (0 == strcmp(pcAttr, "group")) {
		if ((struct group *)0 == (gr = getgrgid(pstThis->st_gid)))
			fatal("getgrgid: %ld: no such group", (long)pstThis->st_gid);
		snprintf(acTarget, sizeof(acTarget), "%s", gr->gr_name);
	} else if (0 == strcmp(pcAttr, "mode")) {
		snprintf(acTarget, sizeof(acTarget), "%04lo", (long)(07777&pstThis->st_mode));
	} else {
		fatal("%%f.%s: unknown attribute", pcAttr);
	}
	return GenMatch(pcAttr, acTarget, pcRE);
}

/* Approve the direct file and return the full-path to it.		(ksb)
 * By the way: -f "" == -f `pwd`.
 */
static char *
DirectFile(const cmd_t *cmd, char *pcThis, struct stat *pstThis)
{
	register char *pcLim, *pcAttr, *pcFull;
	register int fRemove, iLen, iCheck;
	register const char *pcRead;
	auto struct stat stFull;
	auto char acLook[MAXPATHLEN+256+4];
	auto char acTail[256+4];
	static char acFPerms[] = "-f: Permission denied";

	/* Remove trailing slashes from the input filename, which is not NULL
	 */
	if ((char *)0 == pcThis) {
		/* premission denied error in caller */
		return (char *)0;
	}
	while ((char *)0 != (pcLim = strrchr(pcThis, '/')) && pcLim > pcThis && '\000' == pcLim[1]) {
		*pcLim = '\000';
	}

	/* When the file doesn't exist, or is an allowed symbolic link
	 * compress the path w/o the last component.
	 * When they OK a symbolic link (or mandaite it) then we allow it,
	 * otherwise map it to the contents of the link
	 */
	acTail[0] = '\000';
	fRemove = 0;	/* 0 = no, 1 = was_tail_link, 2 = no_such_file */
	if (-1 == lstat(pcThis, pstThis)) {
		fRemove = 2;
	} else if (S_ISLNK(pstThis->st_mode)) {
		fRemove = 1;
		if ((char *)0 != (pcRead = FindOpt(cmd, "%f.type"))) {
			fRemove = !GenMatch("type", "l", pcRead);
		}
		if ((char *)0 != (pcRead = FindOpt(cmd, "!f.type")) && GenMatch("type", "l", pcRead)) {
			fRemove = 1;
		}
	}
	if (0 != fRemove) {
		if ((char *)0 == pcLim) {
			(void)strncpy(acTail, pcThis, sizeof(acTail));
			pcThis = ".";
		} else if (pcLim == pcThis) {
			(void)strncpy(acTail, pcThis+1, sizeof(acTail));
			pcThis[1] = '\000';
		} else {
			(void)strncpy(acTail, pcLim+1, sizeof(acTail));
			do {
				*pcLim-- = '\000';
			} while (pcThis < pcLim && '/' == *pcLim);
		}
	}
	if ('/' == pcThis[0]) {
		acLook[0] = '/';
		acLook[1] = '\000';
		while ('/' == *pcThis)
			++pcThis;
	} else if ((char *)0 == getcwd(acLook, sizeof(acLook))) {
		fatal("getcwd: %s", strerror(errno));
	}
	if ((char *)0 == (pcLim = FullPath(acLook, pcThis))) {
		fatal("$f: no such path");
	}
	/* Put the part we took off back on then end, if we did that
	 */
	if (0 != fRemove) {
		iLen = strlen(pcLim);
		if (strlen(acTail)+1+iLen > sizeof(acLook))
			fatal("-f resolved path too long");
		/* checked above, don't need any strncat check below */
		if ('/' != pcLim[iLen-1]) {
			(void)strcat(pcLim, "/");
		}
		(void)strcat(pcLim, acTail);
	}
	iCheck = lstat(pcLim, & stFull);
	switch (fRemove) {
	case 0:	/* must be the same node */
		if (-1 == iCheck) {
			fatal("-f: fullpath doesn't find any file at %s", pcLim);
		}
		if (stFull.st_dev != pstThis->st_dev || stFull.st_ino != pstThis->st_ino) {
			fatal("-f: %s is not the same file %s", pcThis, pcLim);
		}
		break;
	case 1: /* must be the same symbilic link */
		if (!S_ISLNK(stFull.st_mode)) {
			fatal("-f: %s: fullpath is not a symbolic link", pcLim);
		}
		if (stFull.st_dev != pstThis->st_dev || stFull.st_ino != pstThis->st_ino) {
			fatal("-f: %s is not the same symbolic link %s", pcThis, pcLim);
		}
		break;
	case 2: /* should still not exist */
		if (-1 != iCheck) {
			fatal("-f: missing file race for %s", pcLim);
		}
		pstThis = (struct stat *)0;
		break;
	}
	pcFull = strdup(pcLim);
	for (pcAttr = asFKnown; '\000' != *pcAttr; pcAttr += strlen(pcAttr)+1) {
		snprintf(acLook, sizeof(acLook), "%%f.%s", pcAttr);
		if ((char *)0 != (pcLim = FindOpt(cmd, acLook))) {
			if (!FileAttr(pcFull, pstThis, pcLim, pcAttr))
				fatal(acFPerms);
		}
		snprintf(acLook, sizeof(acLook), "!f.%s", pcAttr);
		if ((char *)0 != (pcLim = FindOpt(cmd, acLook))) {
			if (FileAttr(pcFull, pstThis, pcLim, pcAttr))
				fatal(acFPerms);
		}
	}
	return pcFull;
}

/* Allow the configuration to specfiy a forced input/output fd.		(ksb)
 * Accpet: <file  >file  <>file >>file
 * as:	RDONLY, WRONLY, RDWR, RW|O_APPEND (like the shell)
 *
 * We do _NOT_ create files.  That's the point.  The empty file is /dev/null.
 * We will try to connect to a socket, for wrapper logic.  How nice.
 */
static int
PrivOpen(const cmd_t *cmd, const char *pcRedirExpected, const char *pcSpec, const char *pcSecPath)
{
	register const char *pcMode;
	register int iOpts, iRet, iLen;
	auto struct sockaddr_un run;

	if ('<' == *pcSpec || '>' == *pcSpec) {
		pcMode = pcSpec;
		if ('>' == *++pcSpec)
			++pcSpec;
	} else {
		pcMode = pcRedirExpected;
	}
	iOpts = '<' == pcMode[0] ? ('>' == pcMode[1] ? O_RDWR : O_RDONLY) : ('>' == pcMode[0] ? ('>' == pcMode[1] ? (O_APPEND|O_WRONLY) : O_WRONLY|O_TRUNC) : O_RDONLY);
	if (0 == strcmp("%f", pcSpec)) {
		if ((char *)0 == pcSecPath) {
			fatal("%s redirection requires -f", cmd->pcname);
		}
		pcSpec = pcSecPath;
	}
	if ('\000' == *pcSpec) {
		pcSpec = acDevNull;
		if (O_RDONLY == iOpts)
			iOpts = O_RDWR;
	}
	if (-1 != (iRet = open(pcSpec, iOpts, 0600))) {
		return iRet;
	}
	if (EOPNOTSUPP != errno) {
		fatal("%s: open: %s: %s", cmd->pcname, pcSpec, strerror(errno));
	}
	if ((iLen = strlen(pcSpec)) >= sizeof(run.sun_path)) {
		fatal("%s: connect: %s", cmd->pcname, strerror(ENAMETOOLONG));
	}
	if (-1 == (iRet = socket(AF_UNIX, SOCK_STREAM, 0))) {
		fatal("%s: socket: %s", cmd->pcname, strerror(errno));
	}
#if NEED_SUN_LEN
	run.sun_len = iLen;
	++iLen;
#endif
	run.sun_family = AF_UNIX;
	(void)strncpy(run.sun_path, pcSpec, sizeof(run.sun_path));
	if (-1 == connect(iRet, (struct sockaddr *)&run, iLen+sizeof(run.sun_family))) {
		fatal("%s: connect: %s: %s", cmd->pcname, pcSpec, strerror(errno));
	}
	/* We could shutdown the socket to honor the redir, nah.
	 */
	return iRet;
}

/* Find the right slot the current value, and the current list.		(ksb)
 * The list holds the max count hidden in ppcList[-1], since the environment
 * before the first slot is never used by anyone else :-).  We find a proposed
 * value from the OldEnv (if we have one) then see is we can overwrite an
 * old value in the list, else we add it on the end.
 *
 * Init the list to a (const char **)0, then call me to add elements.
 */
static void
AddVec(const char ***pppcList, const char *pcEnv, char **ppcOldEnv, int fUniq)
{
#define DEF_ENV_SLOT	768	/* cannot be 0				*/
#define ADD_ENV_SLOTS	256	/* how many to add as we run out	*/
	register int j, iMax, iWas, iFound;
	register size_t iLen;
	register char const **ppcSetEnv, *pcUserEnv, *pcEq;
	register char **ppcMem;

	if ((const char **)0 == (ppcSetEnv = *pppcList)) {
		iWas = 0;
		iMax = DEF_ENV_SLOT;
		ppcMem =  (char **)calloc(iMax+1, sizeof(char *));
		if ((char **)0 == ppcMem)
			fatal(acBadAlloc);
		if ((char *)0 == (*ppcMem = (char *)malloc(OP_MAX_LDWIDTH)))
			fatal(acBadAlloc);
		snprintf(ppcMem[0], OP_MAX_LDWIDTH, "%d", iMax);
		ppcSetEnv = (const char **)(ppcMem+1);
	} else {
		iWas = iMax = strtol(ppcSetEnv[-1], (char **)0, 0);
	}

	if ((const char *)0 == pcEnv) {
		iLen = 0;
	} else if ((const char *)0 != (pcEq = strchr(pcEnv, '='))) {
		iLen = pcEq-pcEnv;
	} else {
		iLen = strlen(pcEnv);
	}
	for (iFound = 0; (const char *)0 != pcEnv && (const char *)0 != (pcEq = ppcSetEnv[iFound]); ++iFound) {
		if (fUniq && 0 == strncmp(pcEq, pcEnv, iLen) && ('\000' == pcEq[iLen] || '=' == pcEq[iLen]))
			break;
	}

	/* If we didn't have a value given for the variable, look for
	 * one from the OldEnv, if there is not one clear the request.
	 */
	if ((const char *)0 == pcEnv) {
		/* trying to put the (char *)0 on the end, I guess */
	} else if (fUniq && '\000' == pcEnv[iLen]) {
		pcUserEnv = (const char *)0;
		for (j = 0; (char **)0 != ppcOldEnv && (const char *)0 != (pcUserEnv = ppcOldEnv[j]); ++j) {
			if (0 == strncmp(pcUserEnv, pcEnv, iLen) && '=' == pcUserEnv[iLen])
				break;
		}
		pcEnv = pcUserEnv;
	}
	if ((const char *)0 == pcEnv) {
		/* nothing */
	} else if ((const char *)0 != ppcSetEnv[iFound]) {
		ppcSetEnv[iFound] = pcEnv;
	} else {
		if (iFound == iMax-1) {
			iMax += ADD_ENV_SLOTS;
			ppcMem = (char **)realloc((void *)(ppcSetEnv-1), sizeof(char *)*iMax);
			if ((char **)0 == ppcMem)
				fatal(acBadAlloc);
			snprintf(ppcMem[0], OP_MAX_LDWIDTH, "%d", iMax);
			ppcSetEnv = (const char **)(ppcMem+1);
			for (j = iWas; j < iMax; ++j) {
				ppcSetEnv[j] = (const char *)0;
			}
		}
		ppcSetEnv[iFound] = pcEnv;
	}
	*pppcList = ppcSetEnv;
#undef DEF_ENV_SLOT
#undef ADD_ENV_SLOTS
}

/* Use the same code to build argv and environ
 */
#define AddEnv(Mpppc, MpcEnv, MppcOldEnv) AddVec((Mpppc), (MpcEnv), (MppcOldEnv), 1)
#define AddArg(Mpppc, MpcArg)	AddVec((Mpppc), (MpcArg), (char **)0, 0)

/* Delete instances of an environment varible.				(ksb)
 * Works on the same structure as the AddEnv above.
 */
static void
RmEnv(const char ***pppcList, char *pcEnv)
{
	register const char **ppcRmEnv, *pcCur, *pcEq;
	register size_t iLen;
	register int j, iFold;

	if ((const char **)0 == (ppcRmEnv = *pppcList) || (char *)0 == pcEnv) {
		return;
	}
	if ((char *)0 != (pcEq = strchr(pcEnv, '='))) {
		iLen = pcEq-pcEnv;
	} else {
		iLen = strlen(pcEnv);
	}
	for (iFold = j = 0; (char *)0 != (pcCur = ppcRmEnv[j]); ++j) {
		if (0 == strncmp(pcCur, pcEnv, iLen) && '=' == pcCur[iLen])
			continue;
		ppcRmEnv[iFold++] = pcCur;
	}
	ppcRmEnv[iFold] = (char *)0;
}

/* Add or remove an environment variable based on the the output from	(ksb)
 * a helmet/jacket process.  Commands are whole lines:
 *	#comment	a comment
 *	-VAR		remove this from the environment
 *	$VAR=value	set this in the environment
 *	$VAR		copy this from the old environment
 *	[0-9]+		exit code for failure
 *	.*		why we don't love you, fail for me
 * To be sure to over-write you should remove before adding: we
 * won't document what happens if you try to put in duplicates.
 */
static int
ExtInput(int fdIn, char *pcFrom, const char ***pppcEnviron)
{
	extern char **environ;
	register char *pcLine;
	register unsigned int u;
	register int iRet;
	auto unsigned int uLines;
	auto char *pcEol;

	iRet = 0;
	pcLine = cache_file(fdIn, &uLines);
	for (u = 1; u <= uLines; pcLine += strlen(pcLine)+1, ++u) {
		while (isspace(*pcLine))
			++pcLine;
		switch (*pcLine) {
		case '#':
#if DEBUG
			fprintf(stderr, "%s: %s\n", pcFrom, pcLine);
#endif
			break;
		case '$':
			AddEnv(pppcEnviron, pcLine+1, environ);
			break;
		case '-':
			RmEnv(pppcEnviron, pcLine+1);
			break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			iRet = strtol(pcLine, &pcEol, 0);
			if ('\000' == *pcEol)
				break;
			/*FALLTHOUGH*/
		default:	/* I guess it is misdirected errors? */
			fprintf(stderr, "%s: %s: %s\n", Progname, pcFrom, pcLine);
			iRet = EX_PROTOCOL;
		}
	}
	return iRet;
}

/* Find an unescaped $* or $@ in the string, return the * or @ pointer	(ksb)
 * assumes ${...} doesn't have an dollars, stars, or ats.
 */
static char *
FindDStar(char *pcScan)
{
	while ('\000' != *pcScan) {
		if ('$' != *pcScan++) {
			continue;
		}
		if ('*' == pcScan[0] || '@' == pcScan[0])
			return pcScan;
		if ('\\' == pcScan[0] && '\000' != pcScan[1])
			pcScan += 2;
		else
			pcScan += 1;
	}
	return (char *)0;
}

/* Expand command $_, $1, $2.., $*, $i <initgroups login>, $u -u username,
 * $U (-u uid), $g (-g groupname), $G (-g uid), $$, $0, $f/$F, $| (empty string)
 * then execute the command (overlay this program with it).  This function
 * is broken up into blocks that share a common set of local variables.
 */
static int
Go(cmd_t *cmd, int argc, const char **argv, const char *pcMay)
{
	extern char **environ;
	register char *pcLook;
	register const char *pcRead;
	register int i, j, iHeTail;
	auto char *pcSetGroups;
	auto int iSecFd, iDirFd, iNewGroups, iOrigAccess, aiNewIO[3];
	auto const char *pcExecPath, *pcSecPath, *pcSecDir;
	auto const char *pcSetSession, *pcEndSession;
	auto const char **ppcNewArgs, **ppcNewEnv, **ppcHelmet;
	auto uid_t uReal, uEffective, uOrig, uPriv;
	auto gid_t gReal, gEffective, gOrig, gPriv;
	auto const char *pcShell, *pcNewHome;
	auto gid_t agNewList[OP_GIDLIST_MAX], agOrig[OP_GIDLIST_MAX];
	auto struct stat stSecStat;
	auto NEEDS Need;
	static char *apcMagic[4] = {
		"$S", "-c", "$*", (char *)0
	};

	if (0 != Craving(cmd, & Need)) {
		exit(EX_DATAERR);
	}
	if (argc < Need.lCmdParam+1) {
		fatal(acBadNumber);
	}

	/* The max helmet argv is:
	 *	jacket-path -P					# 2
	 *	pid|helmet-path					# + 1 = 3
	 *	-u user -g group -f file			# + 6 = 9
	 *	-R root -C $cf					# + 4 = 13
	 *	--						# + 1 = 14
	 *	$mnemonic $path $euid:$egid $cred_type:$cred	# + 4 = 19
	 *	(char *)0					# + 1 = 20
	 * so I allocated 48.
	 */
	ppcNewEnv = ppcNewArgs = (const char **)0;
	ppcHelmet = (const char **)0;
	iHeTail = 0;
	pcLook = (char *)0;
	if ((char *)0 != (pcRead = FindOpt(cmd, "helmet")) || ((char *)0 != (pcLook = FindOpt(cmd, "jacket")))) {
		if ((const char **)0 == (ppcHelmet = (const char **)calloc(48, sizeof(const char *))))
			fatal(acBadAlloc);
		ppcHelmet[iHeTail++] = pcLook;
		ppcHelmet[iHeTail++] = "-P";
		ppcHelmet[iHeTail++] = pcRead;
	}

	/* Check the parameter count and $g/$u actions before we commit.
	 * N.B. $* is funny in the code, we have to find the highest explicit
	 * number, $* starts after that (this replaces CountArgs from lex.l).
	 * So $2.$1 $* means replace the $* with "$3" "$4" "$5"... strange
	 * DWIM compression.  More odd $g/$G trip %g on, also $u/$U, $f/$F.
	 */
	{
	if (!fUid && Need.fUid) {
		fatal("%s: command requires -u", cmd->pcname);
	}
	if (!fGid && Need.fGid) {
		fatal("%s: command requires -g", cmd->pcname);
	}
	if (!fSecFile && (Need.fSecFile || Need.fSecDir)) {
		fatal("%s: command requires -f", cmd->pcname);
	}
	if (fUid && !Need.fUid) {
		fatal("Command line -u %s not allowed", pcUid);
	}
	if (fGid && !Need.fGid) {
		fatal("Command line -g %s not allowed", pcGid);
	}
	pcSecPath = pcSecDir = (const char *)0;
	if (fSecFile) {
		register char *pcMem;

		if (!(Need.fSecFile || Need.fSecDir))
			fatal("Command line -f %s not allowed", pcSecFile);
		if ((char *)0 == (pcSecPath = DirectFile(cmd, pcSecFile, &stSecStat)))
			fatal("%s: %s: permission denied", cmd->pcname, pcSecPath);
		if (Need.fSecDir) {
			pcMem = strdup(pcSecPath);
			if ((char *)0 == (pcLook = strchr(pcMem, '/'))) {
				pcMem = ".";
			} else if (pcMem == pcLook) {
				pcMem = "/";
			} else {
				do {
					*pcLook-- = '\000';
				} while (pcLook > pcMem && '/' == *pcLook);
			}
			pcSecDir = pcMem;
		}
	}
	if ((const char **)0 != ppcHelmet) {
		if (fSecFile) {
			ppcHelmet[iHeTail++] = "-f";
			ppcHelmet[iHeTail++] = pcSecPath;
		}
		if (fUid) {
			ppcHelmet[iHeTail++] = "-u";
			ppcHelmet[iHeTail++] = pcUid;
		}
		if (fGid) {
			ppcHelmet[iHeTail++] = "-g";
			ppcHelmet[iHeTail++] = pcGid;
		}
	}

	if (0 == Need.fSecFd) {
		iSecFd = -1;
	} else if (-1 != (iSecFd = open(pcSecPath, O_RDWR|O_NOCTTY, 0600))) {
		/* OK */;
	} else if (!(EPERM == errno || EROFS == errno || EISDIR == errno) || -1 == (iSecFd = open(pcSecPath, O_RDONLY, 0600))) {
		fatal("open: %s: %s", pcSecPath, strerror(errno));
	}
	if (0 == Need.fDirFd) {
		iDirFd = -1;
	} else if (-1 == (iDirFd = open(pcSecDir, O_RDONLY, 0700))) {
		fatal("open: %s: %s", pcSecDir, strerror(errno));
	}
	}

	if ((char *)0 != (pcLook = FindOpt(cmd, "fib"))) {
		register int iFib;
#if HAVE_SETFIB
		extern int setfib(int);	/* FreeBSD lacks extern in 7.2 */
#endif

		iFib = atoi(pcLook);
		if (iFib < 0)
			iFib = 0;
#if HAVE_SETFIB
		if (-1 == setfib(iFib)) {
			fatal("setfib: %d: %s", iFib, strerror(errno));
		}
#else
		if (0 != iFib) {
			fatal("no support for altenate routing tables (FIBs)");
		}
#endif
	}
	if ((char *)0 != (pcLook = FindOpt(cmd, "nice"))) {
		register int iPrio;

		iPrio = atoi(pcLook);
		if (iPrio < -20)
			iPrio = -20;
		if (iPrio > 20)
			iPrio = 20;
		(void)setpriority(PRIO_PROCESS, 0, iPrio);
	}

	/* Setup our uid and gid creds, but don't change them yet
	 */
	{
	register struct passwd *pw;
	register struct group *gr;
	auto char *pcField = (char *)0, *pcMem = (char *)0;
	auto gid_t gMap, *pwAdd;
	auto const char *pcEGid;
	auto char *pcEmptySetGroups;
	register int iLen;

	uEffective = uPriv = geteuid();
	uOrig = getuid();
	if (uOrig == uPriv) {
		uPriv = -1;
	}
	uReal = -1;

	gEffective = gPriv = getegid();
	gOrig = getgid();
	if (gOrig == gPriv) {
		gPriv = -1;
	}
	gReal = -1;

	if (-1 == (iOrigAccess = getgroups(OP_GIDLIST_MAX, agOrig))) {
		fatal("getgroups: %s", strerror(errno));
	}
	pcEmptySetGroups = pcSetGroups = (char *)0;
	iNewGroups = 0;
	if ((char *)0 != (pcEGid = FindOpt(cmd, "egid"))) {
		if (0 == strcmp(acMyself, pcEGid)) {
			gEffective = gOrig;
		} else if (0 == strcmp("%f", pcEGid)) {
			gEffective = stSecStat.st_gid;
		} else if (0 == strcmp("%u", pcEGid)) {
			(void) MapUid(pcUid, cmd, &gEffective);
		} else if (0 == strcmp("%g", pcEGid)) {
			gEffective = MapGid(pcGid, cmd);
		} else if ((struct group *)0 != (gr = getgrnam(pcEGid))) {
			gEffective = gr->gr_gid;
		} else if (isdigit(*pcEGid)) {
			gEffective = atoi(pcEGid);
		} else {
			fatal("invalid effective group: %s", pcEGid);
		}
	}
	pcNewHome = (const char *)0;
	if ((char *)0 == (pcEmptySetGroups = pcLook = FindOpt(cmd, "uid"))) {
		uReal = uEffective;
	} else if (0 == strcmp(acMyself, pcLook) || '\000' == *pcLook) {
		uReal = uOrig;
		if ((char *)0 == pcEGid) {
			gEffective = gOrig;
			pcEGid = "from real gid";
		}
	} else if (0 == strcmp("%f", pcLook)) {
		uReal = stSecStat.st_uid;
	} else if (0 == strcmp("%u", pcLook)) {
		uReal = MapUid(pcUid, cmd, &gMap);
		if ((char *)0 == pcSetGroups)
			pcSetGroups = pcUid;
		if ((char *)0 == pcEGid) {
			gEffective = gMap;
			pcEGid = "from login group of %u";
		}
	} else if ((struct passwd *)0 != (pw = getpwnam(pcLook))) {
		if ((char *)0 == pcSetGroups) {
			pcSetGroups = pcLook;
		}
		if ((const char *)0 == pcNewHome) {
			pcNewHome = strdup(pw->pw_dir);
		}
		if ((char *)0 == pcEGid) {
			gEffective = pw->pw_gid;
			pcEGid = "from login group of uid";
		}
		uReal = pw->pw_uid;
	} else if (!isdigit(*pcLook)) {
		fatal("invalid real uid: %s", pcLook);
	} else {
		uReal = atoi(pcLook);
	}
	if ((char *)0 == (pcLook = FindOpt(cmd, "euid"))) {
		uEffective = uReal;
	} else if (0 == strcmp(acMyself, pcLook)) {
		uEffective = uOrig;
	} else if (0 == strcmp("%f", pcLook)) {
		uEffective = stSecStat.st_uid;
	} else if (0 == strcmp("%u", pcLook)) {
		uEffective = MapUid(pcUid, cmd, &gMap);
		if ((char *)0 == pcSetGroups)
			pcSetGroups = pcUid;
		if ((char *)0 == pcEGid) {
			gEffective = gMap;
			pcEGid = "from login group of %u";
		}
	} else if ((struct passwd *)0 != (pw = getpwnam(pcLook))) {
		if ((char *)0 == pcSetGroups)
			pcSetGroups = pcLook;
		if ((char *)0 == pcEGid) {
			gEffective = gMap;
			pcEGid = "from login group of euid";
		}
		uEffective = pw->pw_uid;
	} else if (!isdigit(*pcLook)) {
		fatal("invalid effective uid: %s", pcLook);
	} else {
		uEffective = atoi(pcLook);
	}
	if ((char *)0 == pcEmptySetGroups) {
		pcEmptySetGroups = pcLook;
	}

	/* An initgroups spec of %u or %f sets by the command-line, and
	 * a spec of "." leaves them, a name does the obvious, else delete
	 * the groups we had.  Let gid= set more as listed.  This is not
	 * what the original code did, but it is more sane.
	 */
	if ((char *)0 == (pcLook = FindOpt(cmd, "initgroups"))) {
		pcSetGroups = (char *)0;
	} else if ('\000' == pcLook[0] && (char *)0 == (pcLook = pcEmptySetGroups)) {
		fatal("initgroups: no uid or euid set");
	} else if (0 == strcmp(acMyself, pcLook)) {
		pcSetGroups = acMyself;
	} else if (0 == strcmp("%f", pcLook)) {
		if ((struct passwd *)0 == (pw = getpwuid(stSecStat.st_uid))) {
			fatal("getpwuid: -f owner #%ld unmapped for initgroups", (long)stSecStat.st_uid);
		}
		pcSetGroups = strdup(pw->pw_name);
		if ((char *)0 == pcEGid) {
			gEffective = stSecStat.st_gid;
			pcEGid = "from %f";
		}
	} else if (0 == strcmp("%u", pcLook)) {
		pcSetGroups = pcUid;
		(void)MapUid(pcSetGroups, cmd, &gMap);
		if ((char *)0 == pcEGid) {
			gEffective = gMap;
			pcEGid = "from login group of %u";
		}
	} else {
		pcSetGroups = pcLook;
		(void)MapUid(pcSetGroups, cmd, &gMap);
		if ((char *)0 == pcEGid) {
			gEffective = gMap;
			iLen = ((24+strlen(pcSetGroups))|15)+1; /* fmt below */
			if ((char *)0 == (pcMem = malloc(iLen)))
				fatal(acBadAlloc);
			snprintf(pcMem, iLen, "from login group of %s", pcSetGroups);
			pcEGid = pcMem;
		}
	}

	/* A session is a pam version of initgroups, setting other resource
	 * limits and details that we otherwise won't set.  We don't set
	 * all the setrlimit(2) values [or even ulimit(3)], let pam do it.
	 * A session of "" turns this off, like pam="" turns off pam auth.
	 * We end the session under "cleanup=.", at the cost of a fork(2).
	 */
	if ((char *)0 == (pcLook = FindOpt(cmd, acOptSession)) || '\000' == *pcLook) {
		pcSetSession = (const char *)0;
	} else if (0 == strcmp(acMyself, pcLook)) {
		pcSetSession = pcPerp;
	} else if (0 == strcmp("%f", pcLook)) {
		if ((struct passwd *)0 == (pw = getpwuid(stSecStat.st_uid))) {
			fatal("getpwuid: -f owner #%ld unmapped for session", (long)stSecStat.st_uid);
		}
		pcSetSession = strdup(pw->pw_name);
	} else if (0 == strcmp("%u", pcLook)) {
		pcSetSession = pcUid;
	} else if (0 == strcmp("%i", pcLook)) {
		pcSetSession = pcSetGroups;
		if ((char *)0 == pcSetSession && (char *)0 == (pcSetSession = pcEmptySetGroups))
			fatal("session: no uid, euid or initgroups to copy");
	} else {
		pcSetSession = pcLook;
	}
	if ((char *)0 == (pcLook = FindOpt(cmd, acOptCleanup)) || '\000' == *pcLook) {
		pcEndSession = (const char *)0;
	} else if (0 == strcmp(acMyself, pcLook)) {
		pcEndSession = pcSetSession;
	} else if (0 == strcmp("%f", pcLook)) {
		if ((struct passwd *)0 == (pw = getpwuid(stSecStat.st_uid))) {
			fatal("getpwuid: -f owner #%ld unmapped for cleanup", (long)stSecStat.st_uid);
		}
		pcEndSession = strdup(pw->pw_name);
	} else if (0 == strcmp("%u", pcLook)) {
		pcEndSession = pcUid;
	} else if (0 == strcmp("%i", pcLook)) {
		pcEndSession = pcSetGroups;
		if ((char *)0 == pcEndSession && (char *)0 == (pcEndSession = pcEmptySetGroups))
			fatal("cleanup: no uid, euid or initgroups to copy");
	} else {
		pcEndSession = pcLook;
	}

	/* When we asked for a gid=list we have to build it, else use initgroups
	 */
	if ((char *)0 == (pcLook = FindOpt(cmd, "gid"))) {
		static const char acOrig[] = "from client's real gid";

		if ((const char *)0 == pcEGid) {
			gEffective = gOrig;
			pcEGid = acOrig;
		}
		if ((char *)0 == pcSetGroups) {
			setgroups(1, & gEffective);
			gReal = gEffective;
		} else if (acMyself == pcSetGroups) {
			/* leave them be */
		} else if (0 != initgroups(pcSetGroups, gEffective)) {
			fatal("initgroups: %s,%ld: %s", pcSetGroups, (long)gEffective, strerror(errno));
		}
		if (-1 == gReal) {
			gReal = gEffective;
		}
	} else {
		/* Start with our effective, if presented, then add any
		 * groups from initgroups list, only if asked, then from
		 * the gid=list.  gid= means gid=.
		 */
		if ('\000' == *pcLook) {
			pcLook = acMyself;
		}
		if ((char *)0 != pcEGid) {
			AddGid(gEffective, & iNewGroups, agNewList);
		}
		if ((char *)0 == pcSetGroups) {
			/* nada we have the whole list */
		} else if (acMyself == pcSetGroups) {
			for (i = 0; i < iOrigAccess; ++i) {
				AddGid(agOrig[i], & iNewGroups, agNewList);
			}
		} else {
			j = util_getlogingr(pcSetGroups, &pwAdd);
			for (i = 0; i < j; ++i, ++pwAdd) {
				AddGid(*pwAdd, & iNewGroups, agNewList);
			}
		}
		for (pcRead = GetField(pcLook, &pcField); OP_GIDLIST_MAX > iNewGroups && (char *)0 != pcRead; pcRead = GetField(pcRead, &pcField)) {
			if (0 == strcmp(acMyself, pcField)) {
				AddGid(gOrig, & iNewGroups, agNewList);
			} else if (0 == strcmp("%f", pcField)) {
				AddGid(stSecStat.st_gid, & iNewGroups, agNewList);
			} else if (0 == strcmp("%u", pcField)) {
				(void) MapUid(pcUid, cmd, &gMap);
				AddGid(gMap, & iNewGroups, agNewList);
			} else if (0 == strcmp("%g", pcField)) {
				AddGid(MapGid(pcGid, cmd), & iNewGroups, agNewList);
			} else if ((struct group *)0 != (gr = getgrnam(pcField))) {
				AddGid(gr->gr_gid, & iNewGroups, agNewList);
			} else if (isdigit(*pcField)) {
				AddGid(atoi(pcField), & iNewGroups, agNewList);
			} else {
				fatal("gid: invalid real group: %s", pcField);
			}
			/* The first group in the gid list is the real
			 */
			if (-1 == gReal) {
				gReal = agNewList[iNewGroups-1];
			}
		}
		if (-1 == gReal) {
			fatal("invalid empty group list");
		}
		if (setgroups(iNewGroups, agNewList) < 0) {
			fatal("setgroups: %s", strerror(errno));
		}
		if ((char *)0 == pcEGid) {
			gEffective = gReal;
			pcEGid = "from first in gid list";
		}
	}
	/* There is no way to tell is setfsuid(2) worked.  So we just
	 * hope for the best.  If we have the same fsuid we might get
	 * the same value back, which is also the error we get if it
	 * failed.  Who knows which it was?  They don't set errno.
	 */
#if HAVE_SETFSUID
	(void)setfsuid(uEffective);
#endif
#if HAVE_SETFSGID
	(void)setfsgid(gEffective);
#endif
	/* If op is setgid and we have a saved gid drop it
	 */
	if (gReal == gEffective) {
		if (setgid(gReal) < 0) {
			fatal("setgid: %ld: %s", (long)gReal, strerror(errno));
		}
	} else if (setregid(gReal, gEffective) < 0) {
		fatal("setregid: %ld,%ld (from gid and %s): %s", (long)gReal, (long)gEffective, pcEGid, strerror(errno));
	}
	} /* end uid gid creds */

	/* Setup any forced file descriptios for std{in,out,err}, which
	 * depends on the code that assures that 0,1,2 are open already.
	 * We open them before we chroot, of course.  Which can make some
	 * chroot's fail (respecting kern.chroot_allow_open_directories).
	 */
	{
	register const char *pcDaemon;

	aiNewIO[0] = aiNewIO[1] = aiNewIO[2] = -1;
	pcDaemon = ((char *)0 != FindOpt(cmd, acOptDaemon)) ? acDevNull : (const char *)0;
	/* We should be able to read from $n, maybe write too, but not
	 * connect to the socket?
	 */
	if ((const char *)0 != (pcRead = FindOpt(cmd, "stdin")) || (const char *)0 != (pcRead = pcDaemon)) {
		aiNewIO[0] = PrivOpen(cmd, "<", pcRead, pcSecPath);
	}
	if ((const char *)0 != (pcRead = FindOpt(cmd, "stdout")) || (const char *)0 != (pcRead = pcDaemon)) {
		aiNewIO[1] = PrivOpen(cmd, ">", pcRead, pcSecPath);
	}
	if ((const char *)0 != (pcRead = FindOpt(cmd, "stderr")) || (const char *)0 != (pcRead = pcDaemon)) {
		aiNewIO[2] = PrivOpen(cmd, ">", pcRead, pcSecPath);
	}

	/* Don't chroot until you are done with the password/group files
	 */
	if ((const char *)0 != (pcRead = FindOpt(cmd, "chroot"))) {
		if (0 == strcmp("%f", pcRead))
			pcRead = pcSecPath;
		else if (0 == strcmp("%d", pcRead))
			pcRead = pcSecDir;

		if ('\000' != *pcRead && 0 > chroot(pcRead))
			fatal("chroot %s: %s", pcRead, strerror(errno));
	}
	if ((const char **)0 != ppcHelmet) {
		if ((char *)0 != pcRead) {
			ppcHelmet[iHeTail++] = "-R";
			ppcHelmet[iHeTail++] = pcRead;
		}
		ppcHelmet[iHeTail++] = "-C";
		ppcHelmet[iHeTail++] = cmd->pcfile;
	}

	/* chdir, no default setting, but an empty spec is slash
	 */
	if ((char *)0 != (pcRead = FindOpt(cmd, "dir"))) {
		if (0 == strcmp("%f", pcRead))
			pcRead = pcSecPath;
		else if (0 == strcmp("%d", pcRead))
			pcRead = pcSecDir;
		else if ('\000' == *pcRead)
			pcRead = "/";
		if (0 > chdir(pcRead))
			fatal("chdir: %s: %s", pcRead, strerror(errno));
	}
	}

	/* The manual says umask never fails.  I suppose that's true.
	 */
	if ((char *)0 == (pcLook = FindOpt(cmd, "umask"))) {
		(void)umask(0022);
	} else {
		register long lMask;
		auto char *pcEnd;

		lMask = strtoul(pcLook, &pcEnd, 8);
		(void)umask((int)lMask);
	}

	{
	register const char *pcRead, *pcEq;
	register char *pcEnv;
	register int iKeep;
	auto char *pcField;

	/* Copy in the user environment, then overlay ours, if any.
	 * environment	   =>  environment=.
	 * environment=RES =>  match each RE as:
	 *	RE contains an '=' => match against whole VAR=value
	 *	RE w/o any '=' => match against name only.
	 */
	if ((char *)0 != (pcLook = FindOpt(cmd, "environment"))) {
		if ('\000' == *pcLook) {
			pcLook = ".";
		}
		pcRead = pcLook;
		while ((char *)0 != (pcRead = GetField(pcRead, &pcField))) {
			pcEq = strchr(pcField, '=');
			for (i = 0; (char *)0 != (pcEnv = environ[i]); ++i) {
				if ((char *)0 == (pcEnv = strchr(pcEnv, '=')))
					continue;
				if ((char *)0 == pcEq)
					*pcEnv = '\000';
				iKeep = GenMatch("environment", environ[i], pcField);
				*pcEnv = '=';
				if (!iKeep) {
					continue;
				}
				AddEnv(&ppcNewEnv, environ[i], (char **)0);
			}
		}
	}
	}

	/* Produce the command template if it was "MAGIC_SHELL"
	 * Magic shell maps $* -> ${max}, defaults to $S -c $*
	 * also the cmd record may be fixed a little, it is magic.
	 * then check $* (now that we have the final argv list).
	 */
	{
	register char *pcMem;
	register int iLen;

	if (argc < Need.lCmdParam+3) {
		register const char **ppcClone;

		if ((const char **)0 == (ppcClone = calloc(((Need.lCmdParam+3)|3)+1, sizeof(const char *)))) {
			fatal(acBadAlloc);
		}
		for (i = 0; i < argc; ++i) {
			ppcClone[i] = argv[i];
		}
		for (/* above */; i < Need.lCmdParam+3; ++i)
			ppcClone[i] = (const char *)0;
		argv = ppcClone;
	}
	if (0 == strcmp(acMagicShell, cmd->args[0])) {
		/* fold all the params given into the last mentioned
		 */
		iLen = 0;
		for (i = Need.lCmdParam+1; i < argc; ++i) {
			iLen += strlen(argv[i]) + 1;
		}
		if ((char *)0 == (pcMem = (char *)malloc((iLen|15)+1))) {
			fatal(acBadAlloc);
		}

		*pcMem = '\000';
		for (i = Need.lCmdParam+1; i < argc; ++i) {
			if (i != Need.lCmdParam+1)
				strcat(pcMem, " ");
			strcat(pcMem, argv[i]);
		}
		argv[Need.lCmdParam+1] = Need.lCmdParam+1 >= argc ? (char *)0 : pcMem;
		argv[Need.lCmdParam+2] = (char *)0;

		/* If the config gave a magic-shell spell let'r rip, else
		 * set the default magic shell template command.
		 */
		if (1 != cmd->nargs) {
			--cmd->nargs, ++cmd->args;
		} else {
			cmd->args = (const char **)apcMagic;
			if (1 == argc) {
				apcMagic[1] = (char *)0;
				cmd->nargs = 1;
			} else {
				cmd->nargs = 3;
			}
		}
		argc = Need.lCmdParam+2;
	}
	if ((char *)0 != (pcLook = FindOpt(cmd, "$*"))) {
		for (j = Need.lCmdParam+1; j < argc; ++j) {
			if (! GenMatch("!*", argv[j], pcLook))
				fatal(acBadParam);
		}
	}
	if ((char *)0 != (pcLook = FindOpt(cmd, "!*"))) {
		for (j = Need.lCmdParam+1; j < argc; ++j) {
			if (GenMatch("$*", argv[j], pcLook))
				fatal(acBadParam);
		}
	}
	}

	/* Build the actual command argument to execute
	 */
	{
	register int c, iCurArg;
	register char *pcMem;
	register int iMax, iLen, fAllowEmpty;
	register const char *pcSplice, *pcScan, *pcD_;
	register int iArgx, iOptx;
	auto char acCvt[OP_MAX_LDWIDTH];	/* "xx%ld" */
	auto char *pcEnd, *pcRaw;
	auto unsigned long ul;
	auto int iRep, fEoa;
	auto char *pcNew;
	auto gid_t gIgnore, gPrint;
	auto uid_t uPrint;
	auto struct group *gr;
	auto struct passwd *pw;
	auto char acBack[4];

	iMax = 10240;
	if ((char *)0 == (pcMem = (char *)malloc(iMax))) {
		fatal(acBadAlloc);
	}
	fAllowEmpty = iCurArg = 0;

	/* Double conditional loop, for options and env variables, expand
	 * skipping $*, $1, $2 and the like set all the forced $env=values
	 * to their expansion.  Then expand the parameters.  We must expand
	 * the envronment first for $S in the args list.  But first we must
	 * exapand $_.  Which is a pain to setup.
	 */
	iLen = ((strlen(cmd->args[0])+3)|15)+1;
	if ((char *)0 == (pcRaw = malloc(iLen))) {
		fatal(acBadAlloc);
	}
	/* crud, $* expands to multiple words, fix that by changing it from
	 *	argv[0] = "prefix$*suffix"
	 * to
	 *	$_="prefix$1"  args[0] = "$_$*suffix"
	 */
	snprintf(pcRaw, iLen, "$_=%s", cmd->args[0]);
	if ((char *)0 != (pcLook = FindDStar(pcRaw))) {
		register char *pcTemp;

		if ((char *)0 == (pcTemp = malloc(iLen))) {
			fatal(acBadAlloc);
		}
		snprintf(pcTemp, iLen, "$_$.$%c%s", ('*' == *pcLook ? '&' : '!'), pcLook+1);
		cmd->args[0] = pcTemp;
		*pcLook = '\000';
		snprintf(pcLook, iLen-strlen(pcRaw), "%ld", Need.lCmdParam+1);
	} else {
		cmd->args[0] = "$_";
	}
	pcD_ = pcShell = (char *)0;
	for (iOptx = -1, iArgx = 0; (-1 == iOptx ? (++iOptx, (char *)0 != (pcScan = pcRaw)) : (iOptx < cmd->nopts && (char *)0 != (pcScan = cmd->opts[iOptx++]))) || (iArgx < cmd->nargs && (char *)0 != (pcScan = cmd->args[iArgx++])); /* double cond */) {
		/* when looking for an environment variable
		 */
		if (0 == iArgx) {
			if (*pcScan++ != '$') {
				continue;
			}
			if ('#' == *pcScan || '@' == *pcScan || '*' == *pcScan || isdigit(*pcScan)) {
				continue;
			}
		}
		iLen = 0, fEoa = 0;
		iRep = -1;
		while ('\000' != (c = (pcMem[iLen++] = *pcScan++))) {
			if (iLen+2 >= iMax) {
				iMax += 4096;
				if ((char *)0 == (pcNew = realloc((void *)pcMem, iMax))) {
					fatal(acBadAlloc);
				}
				pcMem = pcNew;
			}
			if ('$' != c) {
				continue;
			}
			if ('$' == (c = *pcScan++)) {	/* $$ -> $ */
				continue;
			}
			--iLen;
			pcSplice = (const char *)0;
			switch (c) {
			case '.': /* internal only expander -- break word */
				fEoa = 1;
				pcSplice = "";
				break;
			case '!': /* internal only expander -- @+1 */
			case '&': /* internal only expander -- *+1 */
			case '@':
			case '*':
				if (-1 == iRep) {
					fAllowEmpty = '@' == c || '!' == c;
					iRep = Need.lCmdParam;
					if ('!' == c || '&' == c)
						++iRep;
				}
				pcMem[iLen] = '\000';
				if (++iRep >= argc) {
					iRep = -1;
					pcSplice = "";
					break;
				}
				if (0 == iArgx) {
					if (Need.lCmdParam+1 != iRep)
						pcMem[iLen++] = ' ';
				} else {
					fEoa = iRep < argc-1;
				}
				pcSplice = argv[iRep];
				pcScan -= 2;
				break;
			case '#':
				snprintf(acCvt, sizeof(acCvt), "%ld", (long)argc-1);
				pcSplice = acCvt;
				break;
			case '0': /* Easter Egg $0 is the mnemonic name */
			case '1': case '2': case '3':
			case '4': case '5': case '6':
			case '7': case '8': case '9':
				ul = strtoul(pcScan-1, &pcEnd, 10);
				if (argc < ul) {
					fatal("$%lu beyond given argument list", ul);
				}
				pcScan = pcEnd;
				pcSplice = argv[ul];
				fAllowEmpty = 1;
				break;
			case '{':	/* allow interpolation of old ENV */
				if ((char *)0 == (pcEnd = strchr(pcScan, '}'))) {
					fatal("unbalanced curly reference");
				}
				*pcEnd = '\000';
				if ((char *)0 == (pcSplice = getenv(pcScan)))
					pcSplice = "";
				pcScan = pcEnd+1;
				break;
			case '\\':	/* allow tr list */
				acBack[1] = '\000';
				switch (c = *pcScan++) {
				case '\000':
					fatal("%s: end of string in backslash escape, please ask the admin to run \"%s -S\"", cmd->pcname, Progname);
					/*NOTREACHED*/
					exit(EX_SOFTWARE);
				case 'a':
					acBack[0] = '\007'; break;
				case 'b':
					acBack[0] = '\b'; break;
				case 'f':
					acBack[0] = '\f'; break;
				case 'n':
					acBack[0] = '\n'; break;
				case 'r':
					acBack[0] = '\r'; break;
				case 't':
					acBack[0] = '\t'; break;
				case 'v':
					acBack[0] = '\v'; break;
				case 's':
					acBack[0] = ' '; break;
				default:
					acBack[0] = c; break;
				}
				pcSplice = acBack;
				break;
			case '|':	/* empty string */
				pcSplice = "";
				break;
			case 'C':	/* the configuration directory */
				{
				register char *pcTail;

				if ((char *)0 == (pcSplice = strdup(acAccess))) {
					fatal("$%c %s", c, acBadAlloc);
				}
				if ((char *)0 != (pcTail = strrchr(pcSplice, '/'))) {
					pcTail[1] = '\000';
					while ('/' == *pcTail && pcTail > pcSplice)
						*pcTail-- = '\000';
				} else {
					pcSplice = ".";
				}
				}
				break;
			case 'c':	/* the base configuration file */
				pcSplice = acAccess;
				break;
			case 's':	/* { script } payload */
				pcSplice = cmd->pcscript;
				break;
			case 'S':
				if ((char *)0 == pcShell) {
					auto char **ppcHold = environ;
					environ = (char **)ppcNewEnv;
					pcShell = getenv("SHELL");
					environ = ppcHold;
				}
				if ((char *)0 == pcShell) {
					pcShell = acDefShell;
				}
				/* We also use "-e" when perl is the shell
				 * here, like xapply: you asked for magic.
				 */
				if ((char *)0 != strstr(pcShell, "perl")) {
					apcMagic[1] = "-e";
					if ((char *)0 != cmd->pcscript) {
						cmd->args[1] = apcMagic[1];
					}
				}
				pcSplice = pcShell;
				break;
			case '_':	/* target script, or shell */
				if ((const char *)0 == pcD_)
					fatal("$_ may not be used as part of itself");
				pcSplice = pcD_+2;	/* _=<path> */
				break;
			case 'u':	/* -u login */
				/* check passes %u attributes */
				uPrint = MapUid(pcUid, cmd, &gIgnore);
				pcSplice = pcUid;
				break;
			case 'U':
				uPrint = MapUid(pcUid, cmd, &gIgnore);
				snprintf(acCvt, sizeof(acCvt), "%ld", (long)uPrint);
				pcSplice = acCvt;
				break;
			case 'g':	/* -g group */
				/* check passes %g attributes */
				gPrint = MapGid(pcGid, cmd);
				pcSplice = pcGid;
				break;
			case 'G':
				gPrint = MapGid(pcGid, cmd);
				snprintf(acCvt, sizeof(acCvt), "%ld", (long)gPrint);
				pcSplice = acCvt;
				break;
			case 'd':	/* -f's directory */
				if ((const char *)0 == pcSecDir) {
					fatal("no -f to match $d");
				}
				pcSplice = pcSecDir;
				break;
			case 'D':
				if (-1 == iDirFd) {
					fatal("no -f to match $D");
				}
				snprintf(acCvt, sizeof(acCvt), "%ld", (long)iDirFd);
				pcSplice = acCvt;
				break;
			case 'f':	/* -f file */
				pcSplice = pcSecPath;
				break;
			case 'F':
				if (-1 == iSecFd) {
					fatal("no -f to match $F");
				}
				snprintf(acCvt, sizeof(acCvt), "%ld", (long)iSecFd);
				pcSplice = acCvt;
				break;
			case 't':	/* target login */
				if ((struct passwd *)0 != (pw = getpwuid(uReal))) {
					pcSplice = pw->pw_name;
				} else {
					snprintf(acCvt, sizeof(acCvt), "#%ld", (long)uReal);
					pcSplice = acCvt;
				}
				break;
			case 'T':
				snprintf(acCvt, sizeof(acCvt), "%ld", (long)uReal);
				pcSplice = acCvt;
				break;
			case 'i': /* initgroups target */
			case 'I':
				if (acMyself == pcSetGroups || (char *)0 == (pcSplice = pcSetGroups)) {
					pcSplice = pcPerp;
				}
				if ('I' == c && (struct passwd *)0 != (pw = getpwnam(pcSplice))) {
					snprintf(acCvt, sizeof(acCvt), "%ld", (long)pw->pw_uid);
					pcSplice = acCvt;
				}
				break;
			case 'a': /* original group access list */
			case 'A': /* by gid */
				if (++iRep == iOrigAccess) {
					iRep = -1;
					pcSplice = "";
					break;
				}
				if ('a' == c && (struct group *)0 != (gr = getgrgid(agOrig[iRep]))) {
					snprintf(acCvt, sizeof(acCvt), ",%s", gr->gr_name);
				} else {
					snprintf(acCvt, sizeof(acCvt), ",%s%ld", 'a' == c ? "#" : "", (long)agOrig[iRep]);
				}
				pcSplice = acCvt;
				if (0 == iRep) {
					++pcSplice;
				}
				pcScan -= 2;
				break;
			case 'n': /* new group list */
			case 'N': /* by gid */
				if (++iRep == iNewGroups) {
					iRep = -1;
					pcSplice = "";
					break;
				}
				if ('n' == c && (struct group *)0 != (gr = getgrgid(agNewList[iRep]))) {
					snprintf(acCvt, sizeof(acCvt), ",%s", gr->gr_name);
				} else {
					snprintf(acCvt, sizeof(acCvt), ",%s%ld", 'n' == c ? "#" : "", (long)agNewList[iRep]);
				}
				pcSplice = acCvt;
				if (0 == iRep) {
					++pcSplice;
				}
				pcScan -= 2;
				break;
			case 'e':	/* exec suid name ("root" usually) */
			case '~':	/* exec suid home directory ("/") */
				if (-1 == uPriv || (struct passwd *)0 == (pw = getpwuid(uPriv))) {
					break;
				}
				if ('~' == c) {
					pcSplice = pw->pw_dir;
				} else {
					pcSplice = pw->pw_name;
				}
				break;
			case 'E':	/* our original setuid id */
				if (-1 == uPriv)
					break;
				snprintf(acCvt, sizeof(acCvt), "%ld", (long)geteuid());
				pcSplice = acCvt;
				break;
			case 'b':	/* effective group, not usually used */
				if (-1 == gPriv || (struct group *)0 == (gr = getgrgid(gPriv))) {
					break;
				}
				pcSplice = gr->gr_name;
				break;
			case 'B':	/* effective group by gid, symmetry */
				if (-1 == gPriv)
					break;
				snprintf(acCvt, sizeof(acCvt), "%ld", (long)gPriv);
				pcSplice = acCvt;
				break;
			case 'h':	/* ~perp */
				pcSplice = pcPerpHome;
				break;
			case 'H':	/* ~new-real-uid */
				if ((const char *)0 != pcNewHome) {
					/* nada */
				} else if ((struct passwd *)0 != (pw = getpwuid(uReal))) {
					pcNewHome = strdup(pw->pw_dir);
				}
				pcSplice = pcNewHome;
				break;
			case 'l':	/* original login */
				pcSplice = pcPerp;
				break;
			case 'L':
				snprintf(acCvt, sizeof(acCvt), "%ld", (long)uOrig);
				pcSplice = acCvt;
				break;
			case 'p':	/* pam session login */
				pcSplice = pcSetSession;
				break;
			case 'P':	/* pam session uid */
				if ((struct passwd *)0 != (pw = getpwnam(pcSetSession))) {
					snprintf(acCvt, sizeof(acCvt), "%ld", (long)pw->pw_uid);
					pcSplice = acCvt;
				}
				break;
			case 'r':	/* original real group */
				if ((struct group *)0 != (gr = getgrgid(gOrig))) {
					pcSplice = gr->gr_name;
				} else {
					snprintf(acCvt, sizeof(acCvt), "#%ld", (long)gOrig);
					pcSplice = acCvt;
				}
				break;
			case 'R':
				snprintf(acCvt, sizeof(acCvt), "%ld", (long)gOrig);
				pcSplice = acCvt;
				break;
			case 'o':	/* target group */
				if ((struct group *)0 != (gr = getgrgid(gReal))) {
					pcSplice = gr->gr_name;
				} else {
					snprintf(acCvt, sizeof(acCvt), "#%ld", (long)gReal);
					pcSplice = acCvt;
				}
				break;
			case 'O':	/* target gid */
				snprintf(acCvt, sizeof(acCvt), "%ld", (long)gReal);
				pcSplice = acCvt;
				break;
			case 'w':
				pcSplice = cmd->pcfile;
				break;
			case 'W':
				snprintf(acCvt, sizeof(acCvt), "%ld", (long)cmd->iline);
				pcSplice = acCvt;
				break;
			default:
				break;
			}
			if ((const char *)0 == pcSplice) {
				fatal("%s: $%c: no data found", cmd->pcname, c);
			}
			if (iLen+strlen(pcSplice) >= iMax) {
				iMax += (4095|strlen(pcSplice))+1;
				if ((char *)0 == (pcNew = realloc((void *)pcMem, iMax))) {
					fatal("$%c %s", c, acBadAlloc);
				}
				pcMem = pcNew;
			}
			(void)strcpy(pcMem+iLen, pcSplice);
			iLen += strlen(pcSplice);
			if (fEoa && (fAllowEmpty || '\000' != pcMem[0])) {
				AddArg(&ppcNewArgs, strdup(pcMem));
				++iCurArg;
				iLen = 0, fEoa = 0;
			}
		}
		/* don't put our $_ in the env, they might set or pass one
		 */
		if (0 == iOptx) {
			pcD_ = strdup(pcMem);
			continue;
		}
		if (0 == iArgx) {
			AddEnv(&ppcNewEnv, strdup(pcMem), environ);
			continue;
		}
		/* N.B iLen is 1 too big since we counted the \000 */
		if (1 == iLen && !fAllowEmpty) {
			continue;
		}
		fAllowEmpty = 0;
		AddArg(&ppcNewArgs, strdup(pcMem));
		++iCurArg;
	}

	/* When basename is set we lie about argv[0]
	 */
	pcExecPath = ppcNewArgs[0];
	if ((char *)0 != (pcLook = FindOpt(cmd, "basename"))) {
		ppcNewArgs[0] = pcLook;
	}
	}

	/* Make all the processes we were asked for
	 */
	{
	register char **ppcHold;
	auto char acCvt[OP_MAX_LDWIDTH];	/* "%ld" */
	auto pid_t wPid, wReap;
	auto int iStatus, iFdOldErr, aiPipe[2];

	iFdOldErr = 2;
#define RESTORE_STDERR()	do { register int iE = errno; if (2 != iFdOldErr) { dup2(iFdOldErr, 2); } errno = iE; } while (0)
	/* Finish helmet/jacket args with
	 *	-- $mnemonic $path $euid:$egid $cred_type:$cred (char *)
	 * then maybe run helmet to check any other stuff the config wants.
	 */
	if ((const char **)0 != ppcHelmet) {
		ppcHelmet[iHeTail++] = "--";
		ppcHelmet[iHeTail++] = cmd->pcname;
		ppcHelmet[iHeTail++] = pcExecPath;
		snprintf(acCvt, sizeof(acCvt), "%ld:%ld", (long)uEffective, (long)gEffective);
		ppcHelmet[iHeTail++] = strdup(acCvt);
		ppcHelmet[iHeTail++] = pcMay;
		ppcHelmet[iHeTail++] = (char *)0;
	}
	if ((const char **)0 != ppcHelmet && (const char *)0 != ppcHelmet[2]) {
		if (-1 == pipe(aiPipe)) {
			fatal("pipe: %s", strerror(errno));
		}
		switch (wPid = fork()) {
		case -1:
			fatal("fork: %s", strerror(errno));
			/*NOTREACHED*/
		case 0:
			/* Check the helmet to make sure we're safe to ride.
			 * Let's not give the checker our special redirections.
			 */
			(void)close(0);
			(void)open(acDevNull, O_RDONLY, 0666);
			(void)dup2(aiPipe[1], 1);
			(void)close(aiPipe[0]);
			if (-1 != iSecFd) {
				close(iSecFd);
			}
			if (-1 != iDirFd) {
				close(iDirFd);
			}
			for (i = 0; i < 3; ++i) {
				if (-1 != aiNewIO[i])
					close(aiNewIO[i]);
			}
			/* When we drop uid the client can ptrace us to mess
			 * with the return code: so we can't do that.  Bummer.
			 * Never drop uid to the original mortal in a helmet.
			 */
			execve(ppcHelmet[2], (char **)(ppcHelmet+2), (char **)ppcNewEnv);
			fprintf(stderr, "%s: execve: %s: %s\n", Progname, ppcHelmet[2], strerror(errno));
			exit(EX_UNAVAILABLE);
		default:
			(void)close(aiPipe[1]);
			break;
		}
		if (0 != (iStatus = ExtInput(aiPipe[0], "helmet", &ppcNewEnv))) {
			exit(iStatus);
		}
		close(aiPipe[0]);
		while (wPid != (wReap = wait(& iStatus))) {
			if (-1 == wReap) {
				fatal("wait: %s", strerror(errno));
			}
		}
		if (0 == iStatus) {
			/* access allowed */
		} else if (WIFEXITED(iStatus)) {
			exit(WEXITSTATUS(iStatus));
		} else {
			exit(iStatus);
		}
	}

	/* Let's start a detached process from op, under daemon.
	 */
	for (i = 0; i < 3; ++i) {
		if (-1 == aiNewIO[i])
			continue;
		if (iFdOldErr == i)
			iFdOldErr = dup(i);
		if (-1 == dup2(aiNewIO[i], i)) {
			RESTORE_STDERR();
			fatal("dup2: %d: %s", aiNewIO[i], strerror(errno));
		}
		close(aiNewIO[i]);
	}

	if ((char *)0 != FindOpt(cmd, acOptDaemon)) {
		switch ((i = fork())) {
		case -1:
			/* We really need the semantics of the double fork
			 */
			RESTORE_STDERR();
			fprintf(stderr, "%s: fork: %s\n", Progname, strerror(errno));
			exit(EX_UNAVAILABLE);
		case 0:
			break;
		default:
			exit(EX_OK);
		}
#if defined(TIOCNOTTY)
		if (-1 != (i = open(acDevTty, 2))) {
			ioctl(i, TIOCNOTTY, 0);
			close(i);
		}
#else
		setsid();
#endif
	}

	/* Wrap ourself $jacket -P $pid helmet-args, the args slot for the
	 * pid is the helmet program path (or a NULL pointer), overwrite it.
	 */
	if ((const char **)0 != ppcHelmet && (const char *)0 != *ppcHelmet) {
		if (-1 == pipe(aiPipe)) {
			fatal("pipe: %s", strerror(errno));
		}
		switch ((wPid = fork())) {
		case -1:
			RESTORE_STDERR();
			fatal("fork: %s", strerror(errno));
			/*NOTREACHED*/
		case 0:
			(void)close(aiPipe[1]);
			if (0 != (iStatus = ExtInput(aiPipe[0], "jacket", &ppcNewEnv))) {
				exit(iStatus);
			}
			close(aiPipe[0]);
			break;
		default:
			if (-1 != iSecFd) {
				close(iSecFd);
			}
			if (-1 != iDirFd) {
				close(iDirFd);
			}
			dup2(aiPipe[1], 1);
			(void)close(aiPipe[1]);
			(void)close(aiPipe[0]);
			snprintf(acCvt, sizeof(acCvt), "%ld", (long)wPid);
			ppcHelmet[2] = acCvt;
			execve(*ppcHelmet, (char **)ppcHelmet, (char **)ppcNewEnv);
			RESTORE_STDERR();
			fprintf(stderr, "%s: execve: %s: %s\n", Progname, ppcHelmet[0], strerror(errno));
			exit(EX_UNAVAILABLE);
		}
	}

	if ((const char *)0 != pcSetSession) {
#if USE_PAM
		auto struct pam_conv local_conv;
		auto int iStatus, iExit;
		auto pam_handle_t *pPCLocal;

		local_conv.appdata_ptr = (void *)cmd;
		local_conv.conv = OpConv;
		if (PAM_SUCCESS != (iStatus = pam_start(acDefPam, pcSetSession, &local_conv, &pPCLocal))) {
			RESTORE_STDERR();
			fatal("pam_start: %s: %s", pcSetSession, pam_strerror(pPCLocal, errno));
		}
		(void)pam_set_item(pPCLocal, PAM_TTY, acDevTty);
		(void)pam_set_item(pPCLocal, PAM_RHOST, "localhost");
		(void)pam_set_item(pPCLocal, PAM_RUSER, pcPerp);
		switch (iStatus = pam_open_session(pPCLocal, PAM_SILENT)) {
		case PAM_SUCCESS:
			break;
		default:
			RESTORE_STDERR();
			fatal("session: %s: %s", pcSetSession, pam_strerror(pPCLocal, iStatus));
			/*NOTREACHED*/
		}
		/* If you really need the session closed we have to fork,
		 * which I think is a stretch.  Sudo doesn't bother. -- ksb
		 */
		if ((char *)0 != pcEndSession) {
			switch ((wPid = fork())) {
			case -1:
				fatal("fork: %s", strerror(errno));
				/*NOTREACHED*/
			case 0:
				/* don't hold any input/output pipes
				 */
				if (0 == close(0))
					(void)open(acDevNull, O_RDWR, 0666);
				if (0 == close(1))
					(void)open(acDevNull, O_RDWR, 0666);
				iExit = 127;
				while (wPid != (wReap = wait(& iExit))) {
					if (-1 == wReap) {
						fatal("wait: %s", strerror(errno));
					}
				}
				if (0 != strcmp(pcPerp, pcEndSession))
					(void)pam_set_item(pPCLocal, PAM_USER, pcEndSession);
				iStatus = pam_close_session(pPCLocal, PAM_SILENT);
				(void)pam_end(pPCLocal, iStatus);

				/* PAM errors? The show is over, move along.
				 */
				exit(iExit);
			default:	/* child */
				break;
			}
		} else if (PAM_SUCCESS != (iStatus = pam_end(pPCLocal, iStatus))) {
			RESTORE_STDERR();
			fatal("pam_end: %s", pam_strerror(pPCLocal, iStatus));
		}
#else
		fatal("%s: no support on this host", acOptSession);
#endif
	}
	if (uReal == uEffective) {
		if (setuid(uReal) < 0) {
			RESTORE_STDERR();
			fatal("setuid: %ld: %s", (long)uReal, strerror(errno));
		}
	} else if (setreuid(uReal, uEffective) < 0) {
		RESTORE_STDERR();
		fatal("setreuid: %ld,%ld: %s", (long)uReal, (long)uEffective, strerror(errno));
	}
	syslog(LOG_NOTICE, "user %s SUCCEEDED to execute %s", pcPerp, pcCmd);
	execve(pcExecPath, (char **)ppcNewArgs, (char **)ppcNewEnv);
	ppcHold = environ;
	environ = (char **)ppcNewEnv;
	execvp(pcExecPath, (char **)ppcNewArgs);
	environ = ppcHold;
	RESTORE_STDERR();
#undef RESTORE_STDERR
	}

	/* Gads, after all that work we failed to execve (we don't undo the
	 * syslog notice, as we can't very well do that).
	 */
	fprintf(stderr, "%s: execvp: %s: %s\n", Progname, pcExecPath, strerror(errno));
	return EX_UNAVAILABLE;
}

/* The remains of the original main program from David Koblas, kinda
 */
static int
OldMain(const int argc, const char **argv)
{
	register int iFd, iArgs, iFdNull;
	register size_t iLen, iCredLen;
	register cmd_t *cmd;
	register struct passwd *pw;
	register char *pcCred, *pcEnv, *pcMem;
	auto const char *pcMay;
	auto const char *pcWrong;

	/* You can try, but we'll just open std{in,out,err} to /dev/null
	 */
	iFdNull = -1;
	for (iFd = 0; iFd < 3; ++iFd) {
		auto int iJunk;
		if (-1 == fcntl(iFd, F_GETFD, & iJunk)) {
			if (-1 == iFdNull)
				iFdNull = open(acDevNull, O_RDWR, 0666);
			else
				dup(iFdNull);
		}
	}

	/* We shouldn't ever get a NULL if mkcmd works
	 */
	if ((char *)0 == argv[0]) {
		fatal(acBadList);
	}
	if ((char *)0 == (pcEnv = getenv("USER")) || '\000' == *pcEnv) {
		pcEnv = getenv("LOGNAME");
	}
	if ((char *)0 != pcEnv && '\000' != *pcEnv && (struct passwd *)0 != (pw = getpwnam(pcEnv)) && pw->pw_uid == getuid()) {
		/* found them by their preferred name */
	} else if ((struct passwd *)0 == (pw = getpwuid(getuid()))) {
		fprintf(stderr, "%s: getpwuid: %ld: unmapped uid\n", Progname, (long)getuid());
		exit(EX_NOUSER);
	}
	pcPerp = strdup(pw->pw_name);
	pcPerpHome = strdup(pw->pw_dir);

	/* set the global pcCmd for fatal error conditions
	 */
	pcCred = "";
	if ((char *)0 != pcUid && (char *)0 != pcGid) {
		static char acTempl[] = " as %s:%s";

		iLen = ((sizeof(acTempl)+strlen(pcUid)+strlen(pcGid))|15)+1;
		pcCred = malloc(iLen);
		if ((char *)0 != pcCred)
			snprintf(pcCred, iLen, acTempl, pcUid, pcGid);
	} else if ((char *)0 != pcUid) {
		static char acTempl[] = " as login %s";

		iLen = ((sizeof(acTempl)+strlen(pcUid))|15)+1;
		pcCred = malloc(iLen);
		if ((char *)0 != pcCred)
			snprintf(pcCred, iLen, acTempl, pcUid);
	} else if ((char *)0 != pcGid) {
		static char acTempl[] = " as group %s";

		iLen = ((sizeof(acTempl)+strlen(pcGid))|15)+1;
		pcCred = malloc(iLen);
		if ((char *)0 != pcCred)
			snprintf(pcCred, iLen, acTempl, pcGid);
	}

	iCredLen = strlen(pcCred)+2;
	if ((char *)0 != pcSecFile) {
		iCredLen += strlen(pcSecFile)+4;	/* +strlen(" -f ") */
	}
	for (iArgs = 0; iArgs < argc; ++iArgs) {
		iCredLen += 1+strlen(argv[iArgs]);
	}
	iCredLen = (iCredLen|15)+17;
	if ((char *)0 == (pcMem = (char *)malloc(iCredLen))) {
		fatal(acBadAlloc);
	}
	*pcMem = '\000';
	for (iArgs = 0; iArgs < argc; ++iArgs) {
		(void) strcat(pcMem, " ");
		(void) strcat(pcMem, argv[iArgs]);
	}
	pcMem[0] = '\'';
	(void) strcat(pcMem, "'");
	(void) strcat(pcMem, pcCred);
	if ((char *)0 != pcSecFile) {
		(void) strcat(pcMem, " -f ");
		(void) strcat(pcMem, pcSecFile);
	}
	pcCmd = pcMem;

	pcWrong = acBadMnemonic;
	cmd = Find(argv[0], (long)argc, argv, & pcWrong);
	if ((cmd_t *)0 == cmd) {
		fatal("%s: %s", argv[0], pcWrong);
	}
	cmd = Build(cmd);

	if ((char *)0 == (pcMay = Verify(cmd, argc, argv, &pcWrong))) {
		fatal("%s", pcWrong);
	}
	exit(Go(cmd, argc, argv, pcMay));
}

static char
	*pcExtender,		/* ".cf" usually */
	*pcPrimary;		/* "access.cf" usually */

/* Find all the files with the same extender as the primary config.	(ksb)
 */
static int
AnyConfig(const struct dirent *pDIThis)
{
	register const char *pcDot;

	if ((const char *)0 != (pcDot = strrchr(pDIThis->d_name, '.'))) {
		if ((char *)0 == pcExtender)
			return 0;
		return 0 == strcmp(pcDot, pcExtender);
	}
	return (char *)0 == pcExtender;
}

/* Qsort so the first config file read is the primary one: this puts	(ksb)
 * the access.cf DEFAULT for all at the head of the list, if there is
 * one at the top of that file.
 */
static int
PickPrimary(const void *pvLeft, const void *pvRight)
{
	register const char *pcLeft, *pcRight;

	pcLeft = ((struct dirent **)pvLeft)[0]->d_name;
	pcRight = ((struct dirent **)pvRight)[0]->d_name;
	if ((char *)0 != pcPrimary) {
		if (0 == strcmp(pcLeft, pcPrimary))
			return -1;
		if (0 == strcmp(pcRight, pcPrimary))
			return 1;
	}
	return strcmp(pcLeft, pcRight);
}

/* Read the whole config file via the parser.				(ksb)
 */
static int
ReadFile(const char *pcTemplate)
{
	register char *pcSlash, *pcFile;
	register int iCount, iErr, fd;
	auto struct dirent **ppDI;
	auto char acPath[MAXPATHLEN+4];
	auto struct stat statbuf;

	if (strlen(pcTemplate) > sizeof(acPath)) {
		syslog(LOG_ERR, "configuration path too long (%s)", pcTemplate);
		fatal("open: %s: %s", pcTemplate, strerror(ENAMETOOLONG));
		/*NOTREACHED*/
	}

	/* look for other *.cf in that directory -- ksb
	 */
	if ((char *)0 == (pcSlash = strrchr(strcpy(acPath, pcTemplate), '/')) || acPath == pcSlash) {
		syslog(LOG_ERR, "configuration filename missing slash: %s", acPath);
		fatal("path botch for %s", acPath);
	}
	*pcSlash = '\000';
	pcPrimary = pcSlash+1;
	pcExtender = strrchr(pcPrimary, '.');
	iCount = scandir(acPath, & ppDI, (void *)AnyConfig, PickPrimary);
	if (iCount < 1 || 0 != strcmp(pcPrimary, ppDI[0]->d_name)) {
		syslog(LOG_ERR, "missing main configuration %s", pcTemplate);
		fatal("stat: %s: %s", pcTemplate, strerror(ENOENT));
	}
	while (iCount-- > 0) {
		snprintf(pcSlash, &acPath[sizeof(acPath)]-pcSlash, "/%s", ppDI[0]->d_name);
		if ((char *)0 == (pcFile = strdup(acPath)))
			fatal(acBadAlloc);
		++ppDI;

		if (-1 == (fd = open(pcFile, O_RDONLY))) {
			iErr = errno;
			syslog(LOG_ERR, "open: %s: %s", pcFile, strerror(errno));
			fatal("open: %s: %s", pcFile, strerror(iErr));
		}
		if (-1 == fstat(fd, &statbuf) ||
			(statbuf.st_uid != 0) || /* Owned by root */
			((statbuf.st_mode & 0077) != 0)) { /* no perm */
			syslog(LOG_ERR, "Permission problems on %s", pcFile);
			fatal("Permission problems on %s", pcFile);
		}

		OneFile(fd, pcFile);
		(void)close(fd);
	}
	/* We don't free the scandir result, but we execve soon enough
	 */
	return 0;
}
