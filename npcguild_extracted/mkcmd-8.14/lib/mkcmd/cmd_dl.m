# $Id: cmd_dl.m,v 8.30 1999/09/25 18:34:43 ksb Exp $
# After you include `cmd.h' you may include this tempate to provide	(ksb)
# a `dl' command to dymanicaly load commands from a .so file.  Add
#	CMD_DEF_DL
# to your table of commands.
#
# Any C compatible module compiled (on a Sun as)
#	gcc -fpic -c $file_c
# and loaded with
#	ld -Bdynamic -G -o $module.so $files_o
#
# Must contain a structure CDnode table called "CDDynamic".
# See "cmd_dl.h" for a complete list of what you need to fill out.
#
from '<sys/param.h>'
from '<sys/types.h>'
from '<stdio.h>'
from '<dlfcn.h>'

require "cmd_dl.mc"

# N.B. the exploded decls are shared with util_dl.m, be careful.
# This prevents the export of cmd_dl into each daynamic module. -- ksb
#@Explode decls@
%hi
/* $Id: cmd_dl.m,v 8.30 1999/09/25 18:34:43 ksb Exp $
 * Dynamic command loader interface.  Build the commands you want	(ksb)
 * to load into the interp. and put a (struct CDnode) in the .c file
 * called CDDynamic.  Initialize the struct from your command table,
 * or the module util_dl.m makes this really easy.
 */
#if !defined(CMD_PROTO)
typedef struct CDnode {
	struct CMnode *pCMlocal;	/* our local table to contibute	*/
	unsigned int icmds;		/* the number of local entries	*/
	int (*pfiinit)();		/* initialization function	*/
	int (*pfimerge)();		/* merge order function		*/
} CMD_DYNAMIC;
#define CMD_PROTO
#endif
%%

# The name of the dynamic index node (struct CDnode) in the file.so,
# and the mode of dlopen() to request.
key "CDDynamic" 1 init {
	"CDDynamic"
	"RTLD_NOW|RTLD_GLOBAL|RTLD_PARENT|RTLD_NODELETE"
}
#@Remove@

%i
/* So we'll dlopen the file.so,  find the struct with the key CDDynamic's
 * first value [as a symbolc name passed dlsym], call the init function
 * if it is not (int (*)())0, the merge in the commands [if the init
 * function returned true] with the merge function.
 */
static int cmd_dl();
#define CMD_DEF_DL	{ "dl", "dynamically load commands", cmd_dl, CMD_NULL, "file.so"}
%%
