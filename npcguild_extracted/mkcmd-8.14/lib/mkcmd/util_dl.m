# $Id: util_dl.m,v 8.30 1999/09/25 18:34:43 ksb Exp $
# After you include `cmd.m' you may include this tempate to provide	(ksb)
# the parts needed to build a dynamic library loadable command file.
#
# THIS TEMPLATE should be LAST in the requires list for best results.
#
# Any C compatible module compiled (on a Sun as)
#	gcc -fpic -c $file_c
# and loaded with
#	ld -Bdynamic -G -o $module.so $files_o
# may be loaded into a micro shell.
#
# This module makes the key data structure that allows this code
# to be loaded by the interpreter.  Just build a vector of command
# entries called "CMDynamic" and include this module.
#
from '<sys/param.h>'
from '<sys/types.h>'
from '<stdio.h>'
from '<dlfcn.h>'

require "cmd_dl-decls.m" "util_dl.mc"

key "CMD_INFO" 1 init {
	"CMDynamic"
	"sizeof(%K<CMD_INFO>1ev)/sizeof(%K<CMD_INFO>1ev[0])"
	"(int (*)())0"
	"(int (*)())0"
}
