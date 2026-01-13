#!mkcmd
# $Id: unit.m,v 2.4 2000/06/05 13:10:28 ksb Exp $
# An example unit driver for the remote database RPC system:		(ksb)
# this one does nothing at all.
#
from '<sys/types.h>'
from '<sys/param.h>'
from '<sys/time.h>'
from '<sys/stat.h>'
from '<strings.h>'
from '<fcntl.h>'
from '<stdio.h>'
from '<stdlib.h>'

from '"rcds_callback.h"'

require "std_help.m" "std_version.m"
require "unit-post.m"

basename "unit" ""
routine "setup"
init 9 "dump_args(argc, argv);"
cleanup "unit_register(& UCUnit);"

key "version_args" 2 {
	2 "unit_rcsid"
}

%i
static char unit_rcsid[] =
	"$Id: unit.m,v 2.4 2000/06/05 13:10:28 ksb Exp $";
%%

%c
/* setup any dynamic strucutre we need to do our work, called once	(ksb)
 * when the RPC daemon loads us (for the first request)
 */
static int
dump_args(int argc, char **argv)
{
	fprintf(stderr, "%s: setup argc=%d\n", unit_progname, argc);
	while ((char **)0 != argv && (char *)0 != *argv) {
		fprintf(stderr, "%s:  %s\n", unit_progname, *argv++);
	}
	return 0;
}

/* create the unit file and initialize it to our liking			(ksb)
 */
int
unit_init(pcPath, pcUnit, pcOpts)
char *pcPath, pcUnit, *pcOpts;
{
	return 0;
}

/* Update this unit's value for a specific name				(ksb)
 */
int
unit_put(pcPath, pcUnit, pcName, pcValue, ppcOld)
char *pcPath, *pcUnit, *pcName, *pcValue, **ppcOld;
{
	if ((char **)0 != ppcOld) {
		*ppcOld = (char *)0;
	}
	return 0;
}

/* Read this unit's value for a name					(ksb)
 */
int
unit_get(pcPath, pcUnit, pcName, ppcValue)
char *pcPath, *pcUnit, *pcName, **ppcValue;
{
	return 0;
}

/* Search for a NVP, return a list of those that matched		(ksb)
 * When pcMatch is (char *)0 please return all you can.
 */
int
unit_search(pcPath, pcUnit, pcMatch, pppcFound)
char *pcPath, *pcUnit, *pcMatch, ***pppcFound;
{
	return 0;
}

/* info/status request							(ksb)
 * Also used as a control point al-la ioctl(2).
 */
int
unit_info(pcPath, pcUnit, pcCmd, ppcOut)
char *pcPath, *pcUnit, *pcCmd, **ppcOut;
{
	auto char *pcDump;

	if ((char **)0 != ppcOut) {
		*ppcOut = (char *)0;
	} else {
		ppcOut = & pcDump;
	}
	if ((char *)0 == pcCmd || 0 == strcmp("version", pcCmd)) {
		*ppcOut = unit_rcsid;
	}
	return 0;
}

/* clear any cached data and maybe close open fds if you can		(ksb)
 */
int
unit_uncache()
{
	printf("%s: %s: uncache\n", progname, unit_progname);
	return 0;
}

/* get ready to exit this program					(ksb)
 */
int
unit_cleanup()
{
	printf("%s: %s: cleanup\n", progname, unit_progname);
	return 0;
}

static UNIT_CALLS UCUnit = {
	"unit", 2,
	unit_init,
	unit_put,
	unit_get,
	unit_search,
	unit_info,
	unit_uncache,
	unit_cleanup
};
%%
