#!mkcmd
# $Id: example_fts.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
# An example of an interface that uses the file tree search code	(ksb)

require "std_help.m" "std_fts.m" "util_fts.m"

basename "fts" ""

# what to do in the case where no -R was given
every {
	named "puts"
	update "%n(%a);"
	param "target"
	help "output each target one per line"
}

# What to do when we visit each -R node
# the default key calls puts(3) as well, but we want every to
# control what is called incase someone augments it -- ksb
key "fts_visit" 2 {
	"%R<every>n" "%a->fts_path"
}
