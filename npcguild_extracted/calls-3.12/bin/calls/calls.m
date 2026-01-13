# mkcmd script for calls parser
# $Id: calls.m,v 3.11 1999/05/27 17:38:09 ksb Exp $
#
%i
static char *rcsid =
	"$Id: calls.m,v 3.11 1999/05/27 17:38:09 ksb Exp $";
%%
from '"machine.h"'

routine "options" terse "acUsage" vector "apcHelp"
basename "calls" ""
named "pcProg"

# local temp space
string[1024] variable "acTemp"  {
	local
}
char* variable "pcSplit" {
	local
}
string[1024] 'C' {
	hidden named "acCppCmd"
	init 'PATH_CPP'
	help "provide a different path to cpp"
	user 'strcat(%n, " ");'
}
before {
	update 'strcat(%rCn, " ");'
}

# options
boolean 'a' {
	named "bAll"
	help "print all calls in every function body"
}
boolean 'e' {
	named "bExtern"
	update "%n = !%i;%rin = 1;"
	help "index external functions too"
}
function 'f' {
	named "AddFunc"
	from '"calls.h"'
	update "%n(%a, 0, acCmd);"
	param "func"
	help "start calling trace from given function"
}
function 'F' {
	named "AddFunc"
	from '"calls.h"'
	update "if (0 != (pcSplit = strchr(%a, '/'))) {*pcSplit++ = '\\000';}else {pcSplit = acCmd;}%n(%a, 1, pcSplit);"
	param "func/file"
	help "trace from static function in the given C source file"
}
boolean 'i' {
	named "bIndex"
	help "print an index of defined functions"
}
integer 'l' {
	named "iLevels"
	param "levels"
	init "8"
	help "limit the levels of calling graph displayed"
}
boolean 'r' {
	named "bReverse"
	help "reverse the called/caller relation in the output"
}
boolean 'o' {
	named "bOnly"
	update "%n = !%i;%rin = 1;"
	help "list only called functions in index output"
}
boolean 't' {
	named "bTerse"
	help "terse, list only trees that are requested"
}
boolean 'T' {
	named "bTsort"
	help "output a graph which is useful as input to tsort"
}
boolean 'v' {
	named "bVerbose"
	help "be verbose in output graph"
}
integer 'w' {
	named "iWidth"
	init "PAPERWIDTH"
	param "width"
	help "set output width"
}
boolean 'x' {
	named "bHideExt"
	help "do not show external function in graph"
}
boolean 'z' {
	named "fAlpha"
	help "sort the descendants in alpha order"
}
function 'D' {
	param "define"
	update '(void)sprintf(acTemp, "-%l%%s ", %N);(void)strcat(acCppCmd, acTemp);'
	help "as in cpp, set initial definition"
}
function 'U' {
	param "undefine"
	update '(void)sprintf(acTemp, "-%l%%s ", %N);(void)strcat(acCppCmd, acTemp);'
	help "as in cpp, remove initial definition"
}
function 'I' {
	param "directory"
	update '(void)sprintf(acTemp, "-%l%%s ", %N);(void)strcat(acCppCmd, acTemp);'
	help "as in cpp, search given include directory"
}
boolean 'V' {
	named "fLookFor"
	init "LOOK_FUNCS"
	update "%n = LOOK_VARS;"
	help "look for referenced variables also"
}

# the rest...
every {
	named "Process"
	param "files"
	update "if ('-' == %a[0] && '\\000' == %a[1]) {Dostdin();}else {Process(%a, %a);}"
	help "files to construct graph from"
}

zero {
	update 'if (%rVn) {printf("%%s: %%s\\n", %b, rcsid);}'
}
