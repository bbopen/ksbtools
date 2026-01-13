#!mkcd
# $Id: class.m,v 2.2 2000/07/12 00:26:50 ksb Exp $
# the example class interface file					(ksb)
#
# In the same model as WISE's stater we are dynamically loaded to
# install a Customer in the remote customer database.  The master
# index picks a "class" to:
#	+ Set any manditory public fields
#	+ Build whatever linkage we need to get to the next level (viz.rcds)
#
from '<sys/types.h>'
from '<sys/param.h>'
from '<sys/time.h>'
from '<sys/stat.h>'
from '<strings.h>'
from '<fcntl.h>'
from '<stdio.h>'
from '<stdlib.h>'

require "std_help.m" "std_version.m" "util_ppm.m"
# must be the last mkcmd require
require "class-post.m"

from '"rcls_callback.h"'
%i
/* our version information
 */
static char rcsid[] =
	"$Id: class.m,v 2.2 2000/07/12 00:26:50 ksb Exp $";
%%

%c
/* create the login and initalize attributes we must have		(ksb)
 * return 0 for add, others for failure.
 * This is optional.
 */
int
init(argp, pppcProtect, pppcPublic)
create_param *argp;
char ***pppcProtect, ***pppcPublic;
{
	*pppcProtect = argp->ppcprotect;
	*pppcPublic = argp->ppcpublic;
	return 0;
}

/* info/status request							(ksb)
 * Also used as a control point al-la ioctl(2).
 * This is optional.
 */
int
info(char *pcCntl, char **ppcOut)
{
	if ((char **)0 != ppcOut) {
		*ppcOut = rcsid;
	}
	return 0;
}

/* how to unload our cache and data					(ksb)
 */
int
cleanup()
{
	return 0;
}

/* setup any dynamic strucutre we need to do our work, called once	(ksb)
 * when the RPC daemon loads us, same as a unit in the data layer.
 * Must have one to be a class module.
 */
%%
