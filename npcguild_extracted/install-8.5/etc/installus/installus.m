#!mkcmd
# $Id: installus.m,v 2.10 2000/08/02 15:55:22 ksb Exp $
# mkcmd script for installus options, fakes most of install's options.

from '<sys/param.h>'
from '<sys/stat.h>'
from '<pwd.h>'
from '<fcntl.h>'
from '<errno.h>'

from '"argvutil.h"'
from '"control.h"'
from '"machine.h"'
from '"sendfd.h"'

require "util_getlogingr.m" "util_username.m"
require "std_help.m" "std_version.m"
basename "installus" ""

%i
static char *rcsid =
	"$Id: installus.m,v 2.10 2000/08/02 15:55:22 ksb Exp $";
%%

augment action 'V' {
	user "Version();"
}

%h
extern char *pcGuilty, *pcGuiltyPasswd;
%%

%c
char **ppcInstallArgs;
int iInstallArgs;

#if !defined(INSTALLPATH)
#define INSTALLPATH "/usr/local/bin/install"
#endif
%%

#
# recognize all the install options we'll pass by
# accumulating them into an argv
#

%c
static void
AccumAction(cThis)
	int cThis;
{
	char *apcBuf[1], acBuf[3];

	sprintf(acBuf, "-%c", cThis);
	apcBuf[0] = strdup(acBuf);
	ArgvAppend(&iInstallArgs, &ppcInstallArgs, 1, apcBuf);
}

char *pcGuilty = (char *)0,
     *pcGuiltyPasswd = (char *)0;
extern char **environ;

#if HAVE_GETSPNAM
#include <shadow.h>
#endif

/* we need to put 'progname' at the beginning of our InstallArgs, and	(bj)
 * allocate so machines with st00pid realloc don't coredump trying to
 * realloc(0, etc).  this is also a good time to get the passwd entry
 * of the user.
 */
void
FindGuilty()
{
	extern char *getenv(), *getlogin(), *crypt();
	register char *pcUser;
	register struct passwd *pwd;
#if HAVE_GETSPNAM
	register struct spwd *spwd;
#endif

	ppcInstallArgs = (char **) calloc(1, sizeof(char *));
	ppcInstallArgs[0] = progname;
	iInstallArgs = 1;

	pcUser = getlogin();
	if ((char *)0 == pcUser || '\000' == *pcUser) {
		pcUser = getenv("USER");
	}
	if ((char *)0 == pcUser || '\000' == *pcUser) {
		pcUser = getenv("LOGNAME");
	}
	setpwent();
	if ((char *)0 != pcUser && (struct passwd *)0 != (pwd = getpwnam(pcUser)) && pwd->pw_uid == getuid()) {
		pcGuilty = strdup(pcUser);
	} else if ((struct passwd *)0 != (pwd = getpwuid(getuid()))) {
		pcGuilty = strdup(pwd->pw_name);
	} else {
		fprintf(stderr, "%s: getpwuid: %d: cannot locate your passwd entry\n", progname, getuid());
		exit(2);
	}
#if HAVE_GETSPNAM
	setspent();
	if ((struct spwd *)0 == (spwd = getspnam(pcGuilty))) {
		fprintf(stderr, "%s: getspnam: cannot locate your shadow password\n", progname);
		exit(3);
	}
	pcGuiltyPasswd = strdup(spwd->sp_pwdp);
	endspent();
#else
	pcGuiltyPasswd = strdup(pwd->pw_passwd);
#endif
	if (strlen(crypt("xx", "xx")) != strlen(pcGuiltyPasswd) && '$' != pcGuiltyPasswd[0]) {
		fprintf(stderr, "%s: warning, crypted password looks bogus\n", progname);
	}
	endpwent();
}
%%

init 3 "FindGuilty();"

###############################################
# these are clones of all the install options #
###############################################

action '1' {
	named "AccumAction"
	help "keep exactly one backup of the installed file"
}

action 'D' {
	named "AccumAction"
	help "no backup of target file"
}

boolean 'R' {
	excludes "1Drls"
	named "fRemove"
	user "AccumAction(%w);"
	help "remove the given target"
}

action 'c' {
	update "/* no op -- we always copy */"
	help "a no op for compatibility with install"
}

boolean 'd' {
	excludes "1Dls"
	named "fMustDir"
	user "AccumAction(%w);"
	left "pcDest" { }
	help "install a directory (mkdir)"
	exit {
		named "Install"
		update "exit(%n(0, (char **)0, pcDest));"
	}
	boolean 'r' {
		named "fRecurse"
		user "AccumAction(%w);"
		help "build all intervening directories"
	}
}

boolean 'n' {
	named "fTrace"
	user "AccumAction(%w);"
	help "trace execution but do not do anything"
}

action 'q' {
	named "AccumAction"
	help "if install can recover from the error be quiet"
}

action 's' {
	named "AccumAction"
	help "run strip(1) on file"
}

action 'N' {
	named "AccumAction"
	help "this must be the first installation of the file"
}

action 'l' {
	named "AccumAction"
	help "run ranlib(1) on file (for libraries)"
}

boolean 'v' {
	named "fVerbose"
	user "AccumAction(%w);"
	help "run in verbose mode"
}


#################################
# end of cloned install options #
#################################

# split into two processes.  The child will open files relative
# to the directory from which installus was run and with the
# user's UID.  The parent can then chdir() to the destination
# to avoid spoofing and race conditions
#
after {
	named "RopenSplit"
	update "%n(DropEUID);"
}

# wow, this one is black magic.  I almost ran out of chickens. -- bj
#
action 'L' {
	excludes "1dDnqrRlsW"
	update "/* none */"

	boolean 'x' {
		local named "fExclusive"
		help "list the programs that ONLY user owns"
	}
	local char* variable "pcOwnedUser" {
		param "user"
		help "user whose programs to list, default is yours"
	}
	left [ "pcOwnedUser" ] { }
	zero {
		from '"control.h"'
		named "ShowOwned"
		update "%n(pcOwnedUser, \".\", fExclusive);"
	}
	every {
		from '"control.h"'
		named "ShowOwned"
		update "%n(pcOwnedUser, %N, fExclusive);"
		param "dirs"
		help "where to check for owned programs, default is ."
	}
	help "list the programs that user owns"
}

action 'W' {
	excludes "1DnqrRlsL"
	update "/* none */"

	zero {
		from '"control.h"'
		named "ShowOwner"
		update "%n(pcGuilty);"
	}
	every {
		from '"control.h"'
		named "ShowOwner"
		update "%n(%N);"
		param "files"
		help "list maintainers for products"
	}
	help "list owners"
}

zero {
	aborts 'if (!fRemove) {fprintf(stderr, "%%s: no files to install?\\n", %b);exit(2);}'
	update "/* aborts */"
}

list {
	named "Install"
	update "exit(%n(%#, %@, pcDest));"
	param "files"
	help "list of files to install"
}

char* variable "pcDest" {
	local param "destination"
	help "destination filename or directory"
}

right "pcDest" {
}


%c
/* fork/exec a program to do some work, return exit status		(bj)
 */
int
Run(path, av, envp)
char *path;
char **av;
char **envp;
{
	register int iPid;
	auto int iReturn;

	switch (iPid = fork()) {
	case -1:
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		exit(1);
	case 0:
		environ = envp;
		if (-1 == execvp(path, av)) {
			fprintf(stderr, "%s: execvp: %s: %s\n", progname, path, strerror(errno));
			exit(1);
		}
		/* NOTREACHED we hope */
	default:
		break;
	}
#if HAS_WAITPID
	waitpid(iPid, &iReturn, 0);
#else
	{	register int i;
		while (-1 != (i = wait(& iReturn)) && i != iPid)
			/* loop */;
	}
#endif
	return (iReturn >> 8) & 0xff;
}

/* in the modes we can use `-o ~' to force Joe to own the file		(ksb)
 */
void
ReplaceTilde(argc, argv, pcWho)
int argc;
char **argv, *pcWho;
{
	register int i;
	for (i = 0; i < argc; ++i) {
		if ('~' != argv[i][0] || '\000' != argv[i][1])
			continue;
		argv[i] = pcWho;
	}
}

static int iSaveArgs;

/* install a single file from pcSource into pcDestDir/pcDestFile	(bj)
 * note that we assume that the proper ParseOwnersFor() call has
 * already been made.  we look up its entry in the
 * .owners file and find out what options to give install for it.
 * we append those to our current InstallArgs and then add the
 * file and destination before forking off an install to do the
 * work.  return boolean success.
 */
int
InstallFile(pcSource, pcDestDir, pcDestFile)
char *pcSource, *pcDestDir, *pcDestFile;
{
	register int i, fd;
	register char *pcArgs;
	auto char *apcTemp[2];

	if ((char *)0 != pcSource && 0 != strcmp("-", pcSource)) {
		/* replace file descriptor 0 with the file we're installing.
		 */
		if (-1 == (fd = ropen(pcSource, O_RDONLY, 0))) {
			fprintf(stderr, "%s: open: %s: %s\n", progname, pcSource, strerror(errno));
			exit(1);
		}
		close(0);
		if (0 != dup(fd)) {
			fprintf(stderr, "%s: dup(%d) != 0: %s\n", progname, fd, strerror(errno));
			exit(5);
		}
		close(fd);
	}
	/* now we should have the file on stdin, ready to install
	 */

	iInstallArgs = iSaveArgs;

	/* we have the file (if any) on stdin, we know what args
	 * they want, it's time to make sure they can do it and get
	 * the appropriate install options from the .owners file
	 */
	if ((char *)0 == (pcArgs = GetInstallOpts(pcDestFile, fMustDir))) {
		/* we've already told them why it failed,
		 * now we just get out.
		 */
		return 0;
	}
	Argvize(&iInstallArgs, &ppcInstallArgs, pcArgs);
	ReplaceTilde(iInstallArgs, ppcInstallArgs, pcGuilty);

	i = 0;
	if ((char *)0 != pcSource) {
		apcTemp[i] = "-";
		++i;
	}
	apcTemp[i] = pcDestFile;
	++i;
	ArgvAppend(&iInstallArgs, &ppcInstallArgs, i, apcTemp);
	if (fVerbose) {
		register int fChdir;

		/* print a cd command unless the dir is "" or "."
		 * then output the install command line, and
		 * the redirection for the ropen call
		 */
		fChdir = !('\000' == pcDestDir[0] || ('.' == pcDestDir[0] && '\000' == pcDestDir[1]));
		printf("%s:", progname);
		if (fChdir) {
			printf(" (cd %s;", pcDestDir);
		}
		printf(" %s", INSTALLPATH);
		for (i = 1; i < iInstallArgs; ++i) {
			printf(" %s", ppcInstallArgs[i]);
		}
		if (fChdir) {
			printf(")");
		}
		if ((char *)0 != pcSource && 0 != strcmp("-", pcSource)) {
			printf(" <%s", pcSource);
		}
		printf("\n");
	}
	return Run(INSTALLPATH, ppcInstallArgs, (char **)0);
}

/* split the destination path into a path and file component             (bj)
 * if pcDest is a dir, then the acDestFile[0] == '\000'.
 */
static int
SplitDest(pcDest, pcDestDir, pcDestFile)
char *pcDest, *pcDestDir, *pcDestFile;
{
	register int iLen;
	register char *pcTemp, cTemp;
	auto struct stat statbuf;

	if ('\000' == *pcDest) {
		fprintf(stderr, "%s: Empty destination?\n", progname);
		return 0;
	}
	iLen = strlen(pcDest);
	if (iLen+2 > MAXPATHLEN) {
		fprintf(stderr, "%s: %s: Directory name too long\n", progname, pcDest);
		return 0;
	}
	if (-1 != stat(pcDest, &statbuf) && S_ISDIR(statbuf.st_mode) && ! fRemove) {
		strcpy(pcDestDir, pcDest);
		/* it's an invariant that iLen > 0 */
		if ('/' != pcDestDir[iLen - 1]) {
			pcDestDir[iLen++] = '/';
			pcDestDir[iLen] = '\000';
		}
		pcDestFile[0] = '\000';
	} else {
		/* either stat failed (file doesn't exist?) or
		 * it wasn't a directory
		 */
		if ((char *)0 == (pcTemp = strrchr(pcDest, '/'))) {
			pcTemp = pcDest;
		} else {
			++pcTemp;
		}
		strcpy(pcDestFile, pcTemp);
		cTemp = *pcTemp;
		*pcTemp = '\000';
		strcpy(pcDestDir, pcDest);
		*pcTemp = cTemp;
	}
	return 1;
}

/* make -rd work for real						(ksb)
 * if the distination doesn't exist we have to recursively make it
 * else the .owners code can't chdir there to find the .owners...
 * This gets really messy as we have to _follow_ the path down with
 * chdir... and look for symbolic link traps.
 * (If "/" is a symbolic link we are in trouble from the get-go.)
 */
static void
BackBeat(pcIn)
char *pcIn;
{
	register char *pcSlash, *pcTop;
	auto struct stat stLink;
	auto int iRet;

	fRecurse = 0;
	pcTop = pcIn;
	if ('/' == *pcIn) {
		++pcIn;
		if (fVerbose) {
			printf("%s: chdir /\n", progname);
		}
		if (-1 == chdir("/")) {
			fprintf(stderr, "%s: chdir: /: %s\n", progname, strerror(errno));
			exit(1);
		}
	}
	while ('\000' != *pcIn) {
		if ((char *)0 != (pcSlash = strchr(pcIn, '/'))) {
			*pcSlash = '\000';
		}
		if (0 == chdir(pcIn)) {
			/* we win already */
		} else if (-1 != lstat(pcIn, & stLink) || ENOENT != errno) {
			/* we lost */
			fprintf(stderr, "%s: %s: lstat: %s\n", progname, pcTop, strerror(errno));
			exit(1);
		} else {
			if (0 != Install(0, (char **)0, pcIn) || 0 != chdir(pcIn)) {
				exit(ENOENT);
			}
		}
		if (fVerbose) {
			printf("%s: chdir %s\n", progname, pcIn);
		}
		if ((char *)0 != pcSlash) {
			*pcSlash = '/';
			pcIn = pcSlash + 1;
		} else {
			break;
		}
	}
	fRecurse = 1;
}

/* drop effective UID							(bj)
 */
void
DropEUID()
{
#if HAVE_SETREUID
	setreuid(getuid(), getuid());
#else
	setuid(getuid());
#endif
}

/* do the install, of course!						(bj)
 * put the list of files in the target directory (or file)
 */
int
Install(ac, av, pcDest)
int ac;
char **av, *pcDest;
{
	register int iFile, iRet;
	auto char acDestDir[MAXPATHLEN+4], acDestFile[MAXPATHLEN+4];
	auto struct stat stOwners;

	/* we currently have progname and the passed options in
	 * InstallArgs.  we save the current Argc so we can go back
	 * right before each file
	 */
	iSaveArgs = iInstallArgs;

	if (!SplitDest(pcDest, acDestDir, acDestFile)) {
		exit(1);
	}
	if (fRecurse && !fRemove) {
		BackBeat(acDestDir);
	}
	if (!ParseOwnersFor(acDestDir, & stOwners, (char *)0)) {
		/* we told them why, just get out
		 */
		exit(1);
	}

	/* install -R */
	if (fRemove) {
		iRet = InstallFile((char *)0, acDestDir, acDestFile);
		iInstallArgs = iSaveArgs;
		return iRet;
	}
	if ('\000' != acDestFile[0]) {
		/* we have a specific filename
		 */
		switch (ac) {
		case 0:
			/* install -d
			 */
			iRet = InstallFile((char *)0, acDestDir, acDestFile);
			break;

		case 1:
			/* install the one file as the given name
			 */
			iRet = InstallFile(av[0], acDestDir, acDestFile);
			break;

		default:
			/* pcDest was a specific filename, and they're trying
			 * to install multiple files.  oops!
			 */
			fprintf(stderr, "%s: destination is not a directory\n", progname);
			iRet = 1;
			break;
		}
		iInstallArgs = iSaveArgs;
		return iRet;
	}

	/* at this point we know we have a dest dir (acDestDir) and
	 * we want to install all the things in the files vector
         * using the same name as source and destination
	 */
	for (iFile = 0; iFile < ac; ++iFile) {
		register char *pcTail;

		if ((char *)0 == (pcTail = strrchr(av[iFile], '/'))) {
			pcTail = av[iFile];
		} else {
			++pcTail;
		}
		if (0 != (iRet = InstallFile(av[iFile], acDestDir, pcTail))) {
			iInstallArgs = iSaveArgs;
			return iRet;
		}
	}
	iInstallArgs = iSaveArgs;
	return 0;
}

/* detail version info							(ksb)
 */
static void
Version()
{
	printf("%s: owners data from \"%s\"\n", progname, acOwners);
	printf("%s: passwords are hashed with %s\n", progname,
#if USE_MD5
		"md5"
#else
		"DES"
#endif
	);
	printf("%s: username is %s\n", progname, username);
}
%%
