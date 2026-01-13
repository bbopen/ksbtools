#!mkcmd
# $Id: msh.m,v 1.4 1997/10/26 20:17:44 ksb Exp $
# rewrite of the message shell --ksb
from '<sys/param.h>'
from '<fcntl.h>'
from '<signal.h>'
from '<stdio.h>'
from '<pwd.h>'
from '<grp.h>'
from '"machine.h"'

require "std_help.m" "std_version.m" "std_noargs.m"

basename "msh" ""

augment action 'V' {
	user "Version();"
}
%i
static char rcsid[] =
	"$Id: msh.m,v 1.4 1997/10/26 20:17:44 ksb Exp $";
extern int errno;

#if !defined(MSPOOL)
#define MSPOOL	"/usr/spool/msh"
#endif
%%

init "Init();"

# Pure and simple, this is a kludge due to the totally brain-damaged
# ISN.  It does not flush its buffer before disconnecting the
# terminal so we "sleep" a little.  We even paid real money for the
# pile of trash!  Yes, I feel better now :-).  KCS
cleanup "sleep(3);"

string named "acMsgDir" {
	init 'MSPOOL'
}

char* named "pcDefault" {
	param "default"
	init '"Default"'
	help "the default message output to the user"
}
left ["pcDefault"] {
}
zero {
	update "Msh();"
}

%c
static void
Version()
{
	printf("%s: message spool is: %s\n", progname, acMsgDir);
}

extern struct passwd *getpwuid();
extern struct group *getgrgid();

static void
Init()
{
	signal(SIGQUIT,SIG_IGN);
#if defined(SIGTSTP)
	signal(SIGTSTP,SIG_IGN);
#endif
#if defined(SIGTTIN)
	signal(SIGTTIN,SIG_IGN);
#endif
#if defined(SIGTTOU)
	signal(SIGTTOU,SIG_IGN);
#endif
	if (0 == chdir(acMsgDir))
		return;
	fprintf(stderr, "%s: chdir: %s: %s\n", progname, acMsgDir, strerror(errno));
	exit(1);
}


/* find the message for the login name we are running as		(ksb)
 * or the name of the shell link, or a default.  Else a
 * hard coded note.
 */
static void
Msh()
{
	register struct passwd *pwd;
	register struct group *grp;
	register int fd;

	setpwent();
	setgrent();

	close(0);
	fd = -1;
	if ((struct passwd *)0 != (pwd = getpwuid(getuid()))) {
		fd = open(pwd->pw_name, O_RDONLY, 0444);
	}

	if (0 != fd && (struct group *)0 != (grp = getgrgid(pwd->pw_gid))) {
		fd = open(grp->gr_name, O_RDONLY, 0444);
	}
	if (0 != fd) {
		fd = open(progname, O_RDONLY, 0444);
	}
	if (0 != fd) {
		fd = open(pcDefault, O_RDONLY, 0444);
	}
	if (0 == fd) {
		system("/bin/cat");
		return;
	}
	printf("%s: Your shell is the message shell\n", progname);
	printf(" There is no message in the spool directory for your login or group.\n");
	printf(" Contact your system administrator for assistance.\n");
}
