#!/mkcmd
# $Id: std_filter.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
#
# filter files like most UNIX pipe tools do,
# take a list of files or stdin by default
#
# to change filter function name
#	augment every { named "myfunc" }
#
# Use the API keys "filter_file" and "filter_pipe" to change the arguments
# to the filter function (%n, %N) for files, (stdin, "-") for pipes.
# The first in each of those keys is the function to call, of course
# (N.B. "%K<_>/ef;" is the cultural way to "call a key").
#

key "filter_file" 2 initialize {
	"%R<every>n" "%n" "%N"
}
every function {
	named "Filter"
	file variable "fpIn" "pcIn" {
		local
	}
	update "%r{fpIn}N = %N;%r{fpIn}Xxu"
	user "%r{fpIn}XK<filter_file>/ef;"
	param "files"
	help "input files to process"
}

key "filter_pipe" 2 initialize {
	"%R<every>n" "stdin" '"-"'
}
zero {
	update '%K<filter_pipe>/ef;'
	help "process stdin"
}
