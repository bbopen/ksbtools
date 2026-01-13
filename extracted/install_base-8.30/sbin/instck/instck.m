# mkcmd script for instck
# $Id: instck.m,v 8.28 2009/12/10 18:45:46 ksb Exp $
require "std_help.m"

from '<sys/param.h>'
from '<sys/types.h>'
from '<sys/stat.h>'
from '<sys/file.h>'
from '<sysexits.h>'
from '<syslog.h>'
from '<ar.h>'

from '"machine.h"'
from '"configure.h"'
from '"install.h"'
from '"main.h"'
from '"special.h"'
from '"syscalls.h"'
from '"instck.h"'

from '<errno.h>'
from '<pwd.h>'
from '<grp.h>'
from '"gen.h"'

vector "apcHelp"
getenv "INSTCK"
basename "instck" ""

key "instck_paths" 8 init {
	"chgrp"
	"chown"
	"strip"
	"chmod"
	"cmp"
	"rm"
	"rmdir"
	"ln"
	"mv"
	"sh"
}
key "install_path" 8 init {
	"%D/install/pv"		# /usr/local/bin/install, we hope
}

key "purge_path" 8 init {
	"%D/purge/pv"		# /usr/local/etc/purge, or from /usr/local/sbin
}

# if we can't find ranlib run /bin/true on the file...
key "instck_ranlib" 8 init {
	"ranlib" "true"
}
# if we can't find chflags fail with /bin/false
key "instck_chflags" 8 init {
	"chflags" "false"
}
require "instck.mh"

before {
	named "Before"
	update "%n();"
}

# general switches
char* 'C' {
	named "pcSpecial"
	init "CONFIG"
	param "config"
	help "use config to check for special files"
}
boolean 'd' {
	named "fDirs"
	init "1"
	help "do not scan given directories for files"
}
boolean 'i' {
	boolean 'y' {
		named "fYesYes"
		help "fake a `yes' response to each interactive query"
	}
	named "fInteract"
	help "interactively repair the installed files"
}
boolean 'l' {
	named "fLongList"
	help "list left over files (those that were not checked)"
}
boolean 'L' {
	named "fLinks"
	help "record hard links in link format"
}
boolean 'v'{
	named "fVerbose"
	help "be more verbose"
}
boolean 'S' {
	named "bHaveRoot"
	help "run as if we were the superuser"
}
boolean 'q' {
	named "fQuiet"
	init "0"
	help "supress warnings about dangerous unchecked files"
}
# install uses this one, but we do not, so emulate the flag
#boolean 'n' {
#	named "fTrace"
#	help "do not really take any destructive actions"
#}
integer variable "fTrace" {
	init "FALSE"
}

# modes
char* 'm' {
	named "pcMode"
	init "DEFMODE"
	param "mode"
	help "set the default mode for installed files"
}
char* 'o' {
	named "pcGenOwner"
	param "owner"
	help "set the default owner for installed files"
}
char* 'g' {
	named "pcGenGroup"
	param "group"
	help "set the default group for all files"
}

# major modes
boolean 'G' {
	named "fGen"
	exclude "iC"
	boolean 'x' {
		named "fByExec"
		help "separate plain files by execute bit"
	}
	boolean 'R' {
		exclude "d"
		named "fRecurse"
		help "descend into all directories"
	}
	help "generate a check list for the given dirs on stdout"
}
boolean 'V' {
	named "fVersion"
	aborts "break;"
	help "show version information"
}

after {
	named "After"
	update "%n();"
}

list {
	named "DoIt"
	param "files"
	help "check these or generate list for them"
}

exit {
	update "/* cleanup and exit */"
	aborts "Exit();exit(0);"
}

# we are su'd to root
char* variable "pcGuilty" {
}
char* variable "pcDefMode" {
}
integer named "iExitCode" {
	init "EX_OK"
	help "the exit code if nothing fatal happens"
}

%i
#if USE_ELF
#if USE_ELFABI
#include <elf_abi.h>
#else
#include <libelf.h>
#endif
#else
#if USE_SYS_EXECHDR_H
#include <sys/exechdr.h>
#else
#include <a.out.h>
#endif
#endif

#if HAVE_NDIR
#include <ndir.h>
#else
#if HAVE_DIRENT
#if USE_SYS_DIRENT_H
#include <sys/dirent.h>
#else
#include <dirent.h>
#endif
#else
#include <sys/dir.h>
#endif
#endif

/* old.h needs DIR defined before it is includeed
 */
#include "old.h"

char copyright[] =
   "@(#) Copyright 1990 Purdue Research Foundation.\nAll rights reserved.\n\
Modified version by ksb";
%%

%%

static char acMBuf[24];		/* mode buffer				*/

/* setup instck
 */
static void
Before()
{
	/* are we the superuser
	 */
	bHaveRoot = 0 == geteuid();
#if USE_ELF
#if USE_ELFABI
	/* cannot check the library itself */
#else
	if (EV_NONE == elf_version(EV_CURRENT)) {
		fprintf(stderr, "%s: wrong ELF version (%d)\n", progname, EV_CURRENT);
		exit(EX_PROTOCOL);
	}
#endif
#endif
}

/* fix any silly option mismatches					(ksb)
 * ``Be liberal with what you accept, strict in what you output.''
 */
static void
After()
{
	register char *pcDot;	/* joe.group style	*/
	auto int iM, iO;	/* mode buffers		*/

	/* set the files we need, instck the dirs we were asked to, end clean
	 */
	(void)setpwent();
	(void)setgrent();

	if ((char *)0 == pcGenGroup || '\000' == pcGenGroup[0]) {
		pcGenGroup = (char *)0;
	}
	if ((char *)0 == pcGenOwner || '\000' == pcGenOwner[0]) {
		pcGenOwner = (char *)0;
	} else if ((char *)0 != (pcDot = strchr(pcGenOwner, '.'))) {
		if ((char *)0 != pcGenGroup) {
			fprintf(stderr, "%s: group given with both -g and -o?\n", progname);
			exit(EX_USAGE);
		}
		*pcDot++ = '\000';
		if ('\000' != *pcDot) {
			pcGenGroup = pcDot;
		}
	}
	if (bHaveRoot) {
		if ((char *)0 == pcGenGroup) {
			pcGenGroup = DEFGROUP;
		}
		if ((char *)0 == pcGenOwner) {
			pcGenOwner = DEFOWNER;
		}
	}

	InitCfg(pcGenOwner, pcGenGroup);

	pcDefMode = acMBuf;
	acMBuf[0] = '-';
	if ((char *)0 == pcMode || '\000' == pcMode[0]) {
		pcMode = (char *)0;
	} else {
		CvtMode(pcMode, & iM, & iO);
		ModetoStr(pcDefMode+1, iM, iO);
	}
}


/* passed the list of files to handle					(ksb)
 */
static void
DoIt(argc, argv)
int argc;
char **argv;
{
	extern char *getenv();
	argc = ElimDups(argc, argv);

	if (fVersion) {
		static char acInh[] = "inherited";

		printf("%s: version: $Id: instck.m,v 8.28 2009/12/10 18:45:46 ksb Exp $\n", progname);
		printf("%s: configuration file: %s\n", progname, pcSpecial);
#if defined(INST_FACILITY)
		printf("%s: syslog facility: %d\n", progname, INST_FACILITY);
#endif
		printf("%s: defaults: owner=%-10s group=%-10s mode=%s\n", progname, (struct passwd *)0 != pwdDef ? pwdDef->pw_name : acInh, (struct group *)0 != grpDef ? grpDef->gr_name : acInh, (char *)0 != pcMode ? pcDefMode : acInh);
		return;
	}

	if (fGen) {
		GenCk(argc, argv);
		return;
	}

#if defined(INST_FACILITY)
	if (bHaveRoot) {
		extern char *getlogin();
		(void)openlog(progname, 0, INST_FACILITY);
		pcGuilty = getlogin();
		if ((char *)0 == pcGuilty || '\000' == *pcGuilty) {
			pcGuilty = getenv("USER");
			if ((char *)0 == pcGuilty || '\000' == *pcGuilty)
				pcGuilty = getenv("LOGNAME");
			if ((char *)0 == pcGuilty || '\000' == *pcGuilty)
				pcGuilty = "root\?";
		}
	}
#endif

	argc = FilterOld(argc, argv, & CLCheck);
	InstCk(argc, argv, pcSpecial, & CLCheck);

#if defined(INST_FACILITY)
	if (bHaveRoot && fInteract) {
		(void)closelog();
	}
#endif
}

/* checkout for now
 */
static void
Exit()
{
	(void)endpwent();
	(void)endgrent();
	exit(iExitCode);
}
