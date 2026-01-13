# mkcmd driver for mk							(ksb)
# $Id: mk.m,v 5.10 2009/11/20 17:38:54 ksb Exp $
from '<sysexits.h>'
from '<stdlib.h>'
from '<unistd.h>'
from '"machine.h"'

require "std_help.m"

getenv "MK"
basename "mk" ""
mix

integer variable "iRetVal" {
	local initializer "0"
}

char* variable "pathname" {
	help "global for full path to mk"
}
char* named "pcHere" {
	help "temporary file for %J"
}

%ih
#if !defined(LEADCHAR)
#define LEADCHAR	'$'
#endif
%%


init "pathname = argv[0];"

action 'H' {
	update "HelpMe(stdout);"
	aborts "exit(EX_OK);"
	help "display only the on-line expander help text"
}

boolean 'A' {
	named "fFirst"
	help "find the first matched command that succeeds"
}
function "D" {
	named "define"
	parameter "defn"
	update "%n(%a);"
	help "give a definition of an environment variable"
}
function "U" {
	named "undefine"
	parameter "undef"
	update "%n(%a);"
	help "remove a definition for an envoronment variable"
}
boolean "V" {
	named "debug"
	help "output version information, or trace actions"
}
boolean "a" {
	named "fAll"
	help "find all matched commands"
}
boolean "c" {
	named "fConfirm"
	help "confirm the action before running it"
}
char* "d" {
	named "submark"
	parameter "submarker"
	help 'look for the string \"$marker(submarker):\" in file'
}
boolean "i" {
	named "fCase"
	help "ignore case for markers and submarkers"
}
integer "l" {
	parameter "lines"
	named "lines"
	initializer "99"
	help "specify the max number of lines to search"
}
char* "m" {
	named "markstr"
	parameter "marker"
	help 'look for the string \"$marker:\" in file'
}
boolean "n" {
	named "fExec"
	initializer "1"
	help "do not really do it"
}
accum [":"] "e" {
	named "pchExamine"
	parameter "templates"
	help "scan templates before given files"
}
char* "T" {
	named "pcTildeDir"
	initializer "AUXDIR"
	parameter "tilde"
	help "supply a new home directory for system templates"
}
accum [":"] "t" {
	named "pchTemplates"
	parameter "templates"
	help "look for marker in template file"
}
boolean "v" {
	named "fVerbose"
	initializer "1"
	update "%n = 1;"
	help "be verbose"
}
action "s" {
	update "%rvn = 0;"
	help "silent operation"
}

char* 'E' {
	named "pcExpand"
	param "text"
	help "just run this text through the expander for each file"
}

zero {
	from '"mk.h"'
	update 'if (%rVn) {Version();}'
}
every {
	named "process"
	param "files"
	update 'iRetVal += %n(%a);'
	help "search for commands in these files"
}
exit {
	update "AddClean((char *)0);"
	aborts "exit(iRetVal);"
}

%c
static void
HelpMe(FILE *fp)
{
	fprintf(fp, "Marker format: %c marker[(submarker)] [=[~]exit-status] [,resource=[limit][/maximum]] : command [$$] [end-text]\n", LEADCHAR);
	fprintf(fp, "Resources: clock core cpu data fsize rss stack files\n");
	fprintf(fp, "Letter options (upper case version):  %%  a literal %%\n");
	fprintf(fp, " a  -a and -A flags (w/o dash)        b  path to binary (last component)\n");
	fprintf(fp, " c  -c flag (w/o dash)                d  directory part of file (fail if local)\n");
	fprintf(fp, " e  -e specification (w/o -e)         f  file (last component without exender)\n");
	fprintf(fp, " g  goto (last component of file)     h  replace marker (replace submarker)\n");
	fprintf(fp, " i  -i flag (w/o dash)                j  here document name (new here document)\n");
	fprintf(fp, " k  a dollar (%%k%%k)                   l  -l lines (w/o -l)\n");
	fprintf(fp, " m  marker (lower case marker)        n  -n flag (w/o dash)\n");
	fprintf(fp, " o  -%%A%%C%%I%%N%%V (w/o dash)            p  file without extender (last component)\n");
	fprintf(fp, " qc file cur at last c (tail)         r  rcs delta for file (for last component)\n");
	fprintf(fp, " s  submarker (lower case)            t  -t templates (w/o -t)\n");
	fprintf(fp, " uc file after last c (tail)          v  -v/-s flags (w/o dash)\n");
	fprintf(fp, " w  active template dir (test)        x  file after last dot (last component)\n");
	fprintf(fp, " y  type of file (f,d,c,b,p,l,s,w,D)  Yc reject unless type c, or Y~c not type c\n");
	fprintf(fp, " z  name of template (reject unless)  ~  mk template directory\n");
	fprintf(fp, " %c  end-text for this line            ?  end-text from here document end\n", LEADCHAR, LEADCHAR, LEADCHAR);
	fprintf(fp, " *  actual marker marker(submarker)   \\C C-like escapes (\\n, \\, \\oct, %c)\n", LEADCHAR);
	fprintf(fp, " +  engage step mode                  -  last marked line in step mode\n");
	fprintf(fp, " .  reject current file completely    ;  last eligible item\n");
	fprintf(fp, " ^  always reject this item\n");
	fprintf(fp, "Grouping forms:                       |/exp/  trim prefix for here documents\n");
	fprintf(fp, " %c%c end command, start end-text       <mapfile>  replace with mapfile match\n", LEADCHAR, LEADCHAR);
	fprintf(fp, " [exp sep fields]  dicer fields       (exp,ranges)  mixer characters selection\n");
	fprintf(fp, " =/exp1/exp2/  reject unless equal    !/exp1/expr2/  reject unless not equal\n");
	fprintf(fp, " {ENV} value of ${ENV} else reject    number RE match or previous here documents\n");
	fprintf(fp, " #number@seek[%%]fmt[size]  extract data from file, size markup b,l,w,i\n");
}
%%
