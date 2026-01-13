#!mkcmd
# $Id: std_lex.m,v 8.7 1999/06/12 00:43:58 ksb Exp $
# make lex do the list of files we get, default to stdin like a filter

require "std_lex.mc"

key "lex_setwrap" 1 init {
	"lex_setwrap" "%P" "%a"
}
key "lex_argv" 1 init {
	"lex_argv"
	'"-"'
}
key "lex_newfile" 1 init {
	"/* %a %P */"
}

list {
	named ""
	update "%K<lex_setwrap>/ef;"
	param "files"
	help "files to parse"
}
