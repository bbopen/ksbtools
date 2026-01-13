#!mkcmd
# $Id: util_glob.m,v 8.8 1999/06/12 00:43:58 ksb Exp $
#
# Use a global "init" to set "util_glob = your_glob_routine;"
# Then things like "cmd_cd" can glob words.
#
# The IEEE glob function (with fins and extra sauce) is easy to
# interface... the code is left as a brain killer for the user.
# Well, OK, clues:
# from '<glob.h>'
# init 'util_glob = glob_IEEE_with_fins_and_sauce;'
# %c...
#	glob_t GLBleh;
#
#	GLBleh.gl_offs = 0;
#	cc = glob(pcFile, GLOB_NOCHECK|GLOB_NOSORT, NULL, & GLBleh);
#	...GLBeh.gl_pathv...
# Your guess is as good as mine as to the malloc()/free() status
# of the argv you get back.  How do you free it?  I would not trust it.

%hi
extern void (*util_glob_free)();
extern char **(*util_glob)();
%%

%c
/* hook for globbing words						(ksb)
 * By default we take all words litterally, you can write code to
 * do any kinda wildcarding that the local system expects.
 *
 * But, amazingly, the GNU glob code would work without any changes:
 *	init "util_glob = glob_filename;"
 * installs the GNU code with no problems.  Funny that.
 */
static char **
u_util_glob(pcFile)
char *pcFile;
{
	register char **ppcFake;

	ppcFake = (char **)calloc(2, sizeof(char *));
	if ((char **)0 != ppcFake) {
		ppcFake[0] = pcFile;
		ppcFake[1] = (char *)0;
	}
	return ppcFake;
}
char **(*util_glob)() = u_util_glob;

/* cleanup after the glob expander (free memory)			(ksb)
 * default, or the the GNU one
 */
static void
u_util_glob_free(ppcHad)
char **ppcHad;
{
	if ((char **)0 != ppcHad) {
		free((char *)ppcHad);
	}
}
void (*util_glob_free)() = u_util_glob_free;
%%
