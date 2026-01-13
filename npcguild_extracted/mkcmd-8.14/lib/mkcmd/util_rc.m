#!mkcmd
# $Id: util_rc.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
# If you need a file for "run commands" like a lot of user utilities	(ksb)
# do you can open that file with a (FILE *) conversion, or a (char *).
#
# N.B. we set "basename" in mkcmd to get best default results
# (of course std_help.m does this for you).
basename

from "<sys/param.h>"
require "util_home.m" "util_rc.mc" "util_rc.mi"

# Key'd support function:
#	Called through the keys of the same name, as always.

key "RcOpen" 2 initialize {
	"RcOpen" "%b" "& %N" "(char **)0"
}
#  set late
#  set user to "%K<RcOpen>/ef;"
#  FILE *RcOpen(pcBasename, ppcKeep, ppcTemplates);
#	Called to find and open the rc file, ppcKeep gets a malloc'd
#	copy of the path to the open rc file.  Also if *ppcKeep is a
#	full path it is the only one we try, if a basename we try it
#	_instead_of_ pcBasename (which is a default).
#	Copy the key to the triggers unnamed key with
#		key init "RcOpen" {
#			3 "apcScanThis"
#		}
#	to replace the default templates for a single file, Clever.
#	{You'll have to change user to "%ZK/ef;" in that case.}

key "RcInit" 2 initialize {
	"RcInit" "%U"
}
#  set before to "%K<RcInit>/ef;"
#  #define RcInit(Mf)	...
#	Sets the macro flag (Mf) to one.  Used to force the track
#	attribute flag to 1.  This forces the late update of the rc
#	file buffer/option to happen even if it was not triggered by
#	the User.
#

# Global buffers (set after init phase)
#
# Augment or overlay the RcFormats key with sprintf-template rules
# contructing a rc filename (last component) from the input name.
# E.g.: ".%s" for "ksh" builds ".ksh" (".%src" builds ".kshrc")
# these are searched in order.  N.B. they have to be protected from
# the expander (double the percents).
#
key "RcFormats" 2 initialize once {
	"%%s" "%%src" ".%%src"
}
key "pcRcPath" 2 initialize once {
	"~/rc" "~/.rc" "~"
}

#  [no Implementor support]
#  char* variable "pcRcPath" { ... }
#	The path to search for rc files:
#		~/rc:~/.rc:~
#	the unnamed key for this variable holds these three default
#	prefixes.  You can add values to this key to add more
#	prefixes to search:
#	augment key "pcRcPath" {
#		"/usr/local/lib/product"
#	}
#	But the environment variable $RCPATH at runtime overrides all.
#	(But maybe it should only *prefix* the default path? -- ksb)


char* variable "pcRcPath" {
	init getenv "RCPATH"

	# The unnamed key below is a really cool one:			(ksb)
	# Used as "eqv", it turns the pcRcPath key into a
	# colon separated list.
	# [We insert a colon before each value, delete the one from
	#  the first value, compact the list into a string.]

	key 2 initialize {
		"%K<pcRcPath>/i':'1.2-$d"
	}
	before 'if (%xi == %n) {%n = %ZK1eqv;}'
	param "rcpath"
	help "path to search for run command (rc) files"
}

# Any of the above might aught'a be output under -V, how would we do
# that you ask?
#	require "std_version.m"
#	agument key "version_format" {
#		"rc version %%s (path %%s)"
#	}
#	agument key "version_args" {
#		"\"%K<pcRcPath>V\""
#		"pcRcPath"
#	}
