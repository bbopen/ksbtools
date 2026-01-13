#!mkcmd
# $Id: util_fts.m,v 1.1 1997/10/04 17:18:10 ksb Beta $
# code to support the std_fts interface
# see also std_xdev.m

require "util_fts.mc"

key "fts_visit" 2 init {
	"puts" "%a->fts_path"
}

key "fts_def_path" 2 init {
	"."
}
