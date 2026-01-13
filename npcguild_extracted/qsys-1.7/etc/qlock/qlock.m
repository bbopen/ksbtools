#!mkcmd
# lock [-nv] [-d Q] "message"
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
	"$Id: qlock.m,v 1.1 1993/10/04 19:25:02 ksb Exp $";
%%
basename "qlock" ""


integer 's' {
	named "iSeconds"
	param "seconds"
	init "-1"
	help "lock the queue only this long"
}

integer variable "fd" {
	hidden
}

after {
	from "<signal.h>"
	update "fd = LockQ(%rdn);"
	user "(void)signal(SIGINT, Intr);"
}

list {
	named "Tell"
	param "message"
	help "explaination for people blocked on the queue"
}

zero {
	update 'fprintf(stderr, "%%s: must provide a message\\n", %b);'
	abort "exit(3);"
}

exit {
	update "if (-1 == %rsn) {pause();}else{sleep(%rsn);}UnLockQ(fd);"
	abort "exit(0);"
}
%%
extern int errno;
#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif

#include "qfuncs.i"

Intr()
{
	/* nothing */;
}

/* this really should be done as one write, 				(ksb)
 * some qstats might get partial messages (ZZZ use writev?)
 */
static void
Tell(argc, argv)
int argc;
char **argv;
{
	(void)lseek(fd, 0, 0L);
	while (argc > 0) {
		(void)write(fd, argv[0], strlen(argv[0]));
		(void)write(fd, "\n", 1);
		++argv, --argc;
	}
	if (fVerbose) {
		printf("%s: locked `%s\'\n", progname, pcQDir);
		fflush(stdout);
	}
}
