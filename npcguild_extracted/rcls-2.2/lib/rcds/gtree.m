#!mkcmd
# $Id: gtree.m,v 2.0 2000/06/05 23:48:57 ksb Exp $
# An example unit driver for the remote database RPC system:		(ksb)
# this one hides the btree module by other names (like "pref" or "addr")
# so that we can share it's cache and code for every other btree.
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
from '"btree.h"'

require "std_help.m" "std_version.m"
require "gtree-post.m"

basename "pref" ""
routine "setup"
init "GLink(gtree_progname);"

key "version_args" 2 {
	2 "gtree_rcsid"
}
%i
static char gtree_rcsid[] =
	"$Id: gtree.m,v 2.0 2000/06/05 23:48:57 ksb Exp $";
%%

%c
/* take notes on my parent unit
 */
static char acParent[] = "btree";
static int (*pfiInfo)();

/* info/status request							(ksb)
 * Also used as a control point al-la ioctl(2).
 */
static int
gtree_info(pcPath, pcUnit, pcCmd, ppcOut)
char *pcPath, *pcUnit, *pcCmd, **ppcOut;
{
	auto char *pcDump;

	if ((char **)0 != ppcOut) {
		*ppcOut = (char *)0;
	} else {
		ppcOut = & pcDump;
	}
	if ((char *)0 == pcCmd || 0 == strcmp("version", pcCmd)) {
		*ppcOut = gtree_rcsid;
		return 0;
	}
	if ((char *)0 == pcCmd || 0 == strcmp("name", pcCmd)) {
		*ppcOut = gtree_progname;
		return 0;
	}

	if ((int (*)())0 != pfiInfo) {
		return pfiInfo(pcPath, pcUnit, pcCmd, ppcOut);
	}
	return 0;
}

/* my control block
 */
static UNIT_CALLS UCGtree = {
	(char *)0, 2,
	(int (*)())0,	/* init		*/
	(int (*)())0,	/* put		*/
	(int (*)())0,	/* get		*/
	(int (*)())0,	/* search	*/
	gtree_info,	/* info		*/
	(int (*)())0,	/* uncache	*/
	(int (*)())0	/* cleanup	*/
};


/* Link to the dynamic btree module for anything we need		(ksb)
 * local entry's call parent if they don't know what to do
 */
static void
GLink(char *pcName)
{
	/* things we make up
	 */
	UCGtree.pcunit = strdup(pcName);

	/* things as trap and pass unknowns
	 */
	pfiInfo = unit_entry(acParent, "info");

	/* things we pass as-is
	 */
	UCGtree.pfiinit = unit_entry(acParent, "init");
	UCGtree.pfiget = unit_entry(acParent, "get");
	UCGtree.pfiput = unit_entry(acParent, "put");
	UCGtree.pfisearch = unit_entry(acParent, "search");
	UCGtree.pfiuncache = unit_entry(acParent, "uncache");

	/* things we don't pass at all
	 *	btree's cleanup: because rcds will cleanup for us,
	 *	when it unloads btree uncache because a Customer might
	 * only know to call our interface to force a sync
	 */
	UCGtree.pficleanup = (int (*)())0;
	unit_register(& UCGtree);
}
%%
