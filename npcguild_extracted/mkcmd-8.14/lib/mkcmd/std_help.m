# $Id: std_help.m,v 8.9 1999/06/12 00:43:58 ksb Exp $
# This mkcmd script provides the standard in-line help interface for	(ksb)
# the UNIX system.
basename terse "u_terse" vector "u_help"

# This allows the user to get a usage message with a -h option
# or a usage line with any bad option.
int variable "u_loop" { local }
char* variable "u_pch" { local }
action "h" {
	from "<stdio.h>"
	update "for (u_loop = 0%; (char *)0 != (u_pch = a%t[u_loop])%; ++u_loop) {if ('\\000' == *u_pch) {printf(\"%%s: with no parameters\\n\", %b);continue;}printf(\"%%s: usage%%s\\n\", %b, u_pch);}for (u_loop = 0%; (char *)0 != (u_pch = %v[u_loop])%; ++u_loop) {printf(\"%%s\\n\", u_pch);}"
	aborts "exit(0);"
	help "print this help message"
}

badoption {
	from "<stdio.h>"
	update "fprintf(stderr, \"%%s: unknown option `-%%c\\', use `-h\\' for help\\n\", %b, %K<getopt_switches>3v);"
	aborts "exit(1);"
}

# This gives a good error messgae for an option that is given without
# a parameter (when it needs one).
noparameter {
	from "<stdio.h>"
	update "fprintf(stderr, \"%%s: option `-%%c\\' needs a parameter\\n\", %b, %o);"
	aborts "exit(1);"
}
