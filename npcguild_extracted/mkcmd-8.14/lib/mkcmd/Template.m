#!/usr/local/bin/mk
# $Id: Template.m,v 8.3 2000/07/30 16:43:44 ksb Exp $
# $Compile(*): %b %o -mMkcmd %f && %b %o -m%m prog.c && mv -i prog %F
# $Mkcmd(*): ${mkcmd-mkcmd} std_version.m std_help.m %f

# include some C declarations we must have (kept as a unique list)
#from '"file.h"'
#from '<std_stuff.h>'

# If we require another option description (after us, but before the
# next thing on the command line)
# We can require "file.mc" (file.mh, file.mi) for meta C code.
# a percent in column 1 forces expansion of any line, others are plain text.
# The best interface is to require an api.m and let it import meta files.
require "std_version.m" "std_help.m"

%hi
#if !defined(MY_INCLUDE_ONCE_TOKEN)
/* copy macros and typedefs to our header file, and our own include sections
 */
#define MY_INCLUDE_ONCE_TOKEN	1
#endif
%%

# Change any API keys you need to change.
# Some templates we require might use an API key to set variable or
# function names, we can override or add to these keys.
#key "whatever" 1 initializer {
#	"value11" 2 "value4"
#}
#augment key "someother" 2 {
#	"on" "the" "end"
#}

# put in all the names we are know be, the `default' name should
# have no options forced.  This other will force one (or two) options
# preferably capital options like:
#	basename "man" ""
#	basename "whatis" "-f"
#	basename "apropos" "-k"
#	basename otherwise "-X"
#
#	 name	forced options
basename "prog"		""
basename "a.out"	"-b"

# rename some global variables if you need to:
#terse "u_terse"
#usage "Usage"
#vector "u_help"
#routine "main"
#named "progname"		# %b

# change how default names are created
#template "%s_opt"
#prototype "%s_func"

# set mutually exclusive options (-i and -j)
#exclude "ij"

# read options from an environment variable
#getenv "PROG"

# never do this, allow dash options to mix with arguments (yucky results)
#mix


# The 4 lines of code below are support for -V from  std_version.m,
# set rcsid to the string -V should output.
# More information could be added with
#augment action 'V' {
#	user "..."
#}
%i
static char rcsid[] =
	"$Id: Template.m,v 8.3 2000/07/30 16:43:44 ksb Exp $";
%%

# Setup any external processors (level 1 to 10)
# default is 5, odds for drivers, evens for templates and libs
#initializer "tcl_init(...);"
#init 'cmd_init(& CSUser, aCMList, sizeof(CMList)/sizeof(CMD), "prompt> ", 3);'
init "init_facility();"

before {
	# calls `before_func()' to setup data structs
	# you get to code this, or leave this out
}

# for a boolean option `-b' eith select `boolean' or `toggle'
# booleans lock to the non-default value, toggle flip every time
boolean 'b' {
	named "fFlag"
	help "a single letter flag"
}

# for other data type (integer, letter, char*, buffer(string), file)
# select one of these
#	integer		unsigned	long		letter
#	string[100]	char*		double		accumulate[":"]
#	file["r"]	fd["open-opts, mode"]
integer 'i' {
	help "set iComamnd to stuff"
	named "iCommand"	# %n %N or named "buffer" "unconverted"
	parameter "stuff"	# %p for usage diagram, as "[name]" for noparam
#	aborts "C-code-opt"	# exit(1), or run C-code to exit
#	from "include"		# buffer is declared in this file
#	exclude "opts"		# options "opts" can't be given in combo.
#	once			# I exclude myself
#	noparam "C-code"	# if no parameter given, must put param in []
#				# C-code sets the _unconverted_ value
#	hidden			# don't show in -h
#	keep "unconverted"	# if you don't give a name you can still keep
#	initializer [style] "value" # set an initial value
#				# styles are getenv, dynamic, or static
#	local			# use a variable local to main()
#	global			# not local
#	static			# set storage class in C to static
#	stops			# force an imaginary  `--' after us
#	ends "C-opt"		# like an abort option, but successful
#	type 'opt' { ... }	# allow this option after me
#	track "variable-opt"	# was this option provided?
#	update "C-code"		# use this code to convert
#	user "C-code"		# after conversion run me
#	verify "C-code-opt"	# check input data on cmd line
#	before "C-code"		# up front init code
#	after "C-code"		# cleanup C code for this option/variable
#	late			# convert option after all dash options
#				# very useful for files and sockets
#	key [version] [initializer] { ... }
#				# %ZK... this trigger's unnamed key
#	key "name" ['short'] [version] [initializer] { ... }
#				# %K<name> or %Ks where short is a character
#
#	requires "module"	# import module for support
#	mkcmd-type named "name" ["keep"] { ... }
#				# declare a local conversion buffer
#
# if we are a file or fd type we can construct a read routine
# if we are a char*, void*, string (or function!) we can construct a
# routine that does "stateful inspection" as data is passed to it.
#	routine "Name" "acBuffer" {	# function name (%mI) and buffer (%mG)
# for this routine:
#		static			# this function is static
#		local/global		# hide (show) terse and vector
#		prototype "u_%mW_%mI"	# %mP pattern constructs function names
#		terse "u_terse_%n"	# %mT (char *[]) of terse hunk help
#		track "iNum"		# %mN (int) line number of file
#		vector "u_vector_%n"	# %mV (char **[]) of verbose hunk help
#		usage "name"		# %mU (void (fp, hunk)()) to print help
#					  a hunk of 0 is the whole file spec
# for each hunk:
#		aborts "C-code"		# %me if line fails conversion run this
#		after "C-code"		# %ma do this after this hunk
#		before "C-code"		# %mb do this before this hunk
#		comment "string"	# %m# comments lead with this (to \n)
#		help "text"		# %mh manual page help
#		named "func"		# %mn C function to call on each line
#		list "v1" "sep" "v2"...	# %m<digits> the line has these columns
#				"sep" might be an INTEGER for fixed widths
#				a leading "sep" is optional to skip stuff
#		update			# %mc special update to convert line
#		user			# %mu user code after conversion
#		stops			#     make a new hunk w/o a clean ends
#		ends count		# hunk is count lines long (> 0)
#		ends "token"		# %mt this string ends the hunk
#					# (and make a new hunk or return)
# computed escapes
#		# %mf format width for names (in help table)
#		# %ml converted buffers as paramlist ("pcLogin, acEPass, iUid")
#		# %mL parameter names as in %ml ("login, epass, uid, gid")
#		# %mj total cols
#		# %mJ which column are we in?
#		# %mw total hunks
#		# %mW which hunk is current?
#		# %mz fail here and now!
# convenient:
#		# %mg option that encloses us
#		# %M... go back to first hunk
#		# %mx... skip forward one hunk per "x"
#
# Repeat as needed after a stops (or ends) to read more data from
# the file (a user code "%mA" will also pass safely to the next hunk
# reader but may miss an "after" action).
# Mkcmd passes to the next only on the ends token (or count).
#	}
}

# Build your own type.  Leave the "base" flag on if the type you are
# building a conversion from an unconverted character string to the type.
# If it takes the original type (converted) and does something to it
# then take the "base" flag off.
#
#integer type "octal" base key-spec-clients {
#	# same attributes as above PLUS these type attributes:
#	type update "C-state"	# a conversion statement to set %n from %N
#	type init "C-expr"	# an initial value of the type
#	type dynamic		# static inits won't cut it, use update to set
#	type comment "text"	# insert this in the manual page
#	type keep		# all clients need a keep (%N), please
#	type help "text"	# help for the usage function
#	type param "word"	# mnemonic for the new type
#	type verify "C-state"	# code to check %N for a "good" format/value
#
#	key 1 init {
#		# force this key to init the client unamed keys
#	}
# Use a @foreach "key-spec-clients" to itterate through the client triggers
# thatuse your type to declare/define buffers and such.
#}

# for special actions that happen _as_ the option is parsed
# use on of:
#	action		(for no parameter, like -h, or -V)
#	function	(for things that have a parameter, like cu -s)
action 'Q' {
	# update calls Q_func()
	# user does nothing
	aborts "exit(7);"
	help "quit the program with exit value 7"
}

# for things like nice(1) that take -integer
number {
	named "iValue"
	param "value"
	user 'printf("%%d\\n", %n);'
	help "what to do with digits"
}

# for things like `vi +lineno file'
# (the "+" is optional as well as the default value)
escape "+" char* {
	named "pcText"
	param "offset"
	help "lseek offset as a string"
}

### part two -- look at the rest of the line:

after {
	# like before, calls after_func(), or rename it:
	# named "junk"
}
# just like init lists (level 1 to 10)
after 5 "/* sanity check input to this point */"

# stuff about left/right here
# put in buffers for all the fix position data (either looking
# from the left or right sise of the command line).
# a buffer for an integer looks like:
#integer variable "iName" {
#	param "myname"
#	help "message for -h"
#}

# then give the order on the command line from the left
# note the special escape prefixes "%1" "%2" ...
# to get to list in the declaration.  (%1<named> for example)
#left "iName" "acName2" [ "acOpt" ] {
#}
# and/or
#right "iOther" {
#}

# list can convert the rest of the argv array to any type, or pass uncvt'd
#list integer {
#	named "piList" "ppcList"
#	...
#}

# put on one of the types (from the integer hunk above) here
#	every double { ...
# to get a different conversion
every function {
	# calls every_func(char *pcData) for each command line value
	param "sticky-fruits"
	help "not bananas"
}

zero {
	# calls zero_func() if we didn't have any `every's
}

# just like init lists (level 10 to 1)
cleanup "/* shutdown the stuff we started */"

exit {
	# call exit_func() to clean up
	#aborts "C-code"		# usual exit declaration
	#after "C-not-reached"		# usually a comment "/*NOTREACHED*/"
}

%c
/* now we are in C code...
 */

/* code the functions you mentioned above, or
 * extern them (or #include the externs)
 */
init_facility()
{
	printf("init something\n");
}

before_func()
{
	printf("before\n");
}

after_func()
{
	printf("after\n");
}

every_func(pcData)
char *pcData;
{
	static int iCount = 0;
	printf("every %s (%d)\n", pcData, ++iCount);
}

exit_func()
{
	printf("exit (cleanup)\n");
}

zero_func()
{
	printf("zero args\n");
}

Q_func()
{
	printf("Q to you too\n");
}
%%
