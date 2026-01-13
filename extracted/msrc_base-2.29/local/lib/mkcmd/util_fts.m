#!mkcmd
# $Id: util_fts.m,v 8.3 2004/12/15 22:20:18 ksb Exp $
# code to support the std_fts interface
# see also std_xdev.m

require "util_fts.mc"

key "fts_visit" 2 init {
	"puts" "%a->fts_path"
}

key "fts_def_path" 2 init {
	"."
}
