#!mkcmd
# make command interface def for turnin's command line			(ksb)
#
require "std_help.m" "std_version.m"
require "../project/path.m"

init "/* RUNS SET UID ROOT */"
%i
static char rcsid[] =
	"$Id: turnin.m,v 5.1 2000/01/06 15:06:28 ksb Exp $";
%%

%ih
#if !defined(WHAT_KEY)
typedef enum {
	INITUSER,		/* initialize a users account		*/
	LISTPROJ,		/* only list projects for a course	*/
	LISTSUB,		/* only list subscribed courses		*/
	VERSION,		/* give version of the program		*/
	UNKNOWN			/* nothing to do, maybe list tar file	*/
} WHAT;
#define WHAT_KEY	"ksb"
#endif
%%

%c
static WHAT eWhat = UNKNOWN;
%%

getenv "TURNIN"
char* 'c' {
	named "pcCourse"
	param "course"
	help "specify the course to use"
}
char* 'p' {
	param "project"
	named "pcLineProj"
	help "specify the project to submit to"
}

exclude "ils"
action 'i' {
	update "eWhat = INITUSER;"
	help "output shell commands to set user's environment"
}
action 'l' {
	update "eWhat = LISTPROJ;"
	help "list projects active for the selected course"
}
action 's' {
	update "eWhat = LISTSUB;"
	help "list courses the envoker is enrolled in"
}

boolean 'v' {
	named "fVerbose"
	help "be verbose (also passed to tar)"
}

after {
	update 'if (0 == %# && UNKNOWN == eWhat && 0 == %rvn) {fprintf(stderr, %t, %b);exit(1);}'
}

%i
extern int Turnin(/* int, char **, what */);
%%
list {
	named "Turnin"
	param "files"
	update "%n(%#, %a, eWhat);"
	help "list of files to submit"
}
