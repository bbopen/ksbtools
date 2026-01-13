#!mkcmd
# $Id: util_by.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
#
# Add which version of mkcmd built the tool to the std_version		(ksb)
# description.

require "std_version.m"

augment key "version_format" 2 {
	"built with %W version %V"
}
