#!mkcmd
# $Id: getopt.m,v 8.11 2000/05/31 13:00:59 ksb Exp $
# code for mkcmd's version of getopt, which is better in that		(ksb)
# it does all the normal things plus the "+option" and "-number" hacks.
# It also knows that "::" means the parameter is optional.  I don't much
# like that, but other people can't live without out.  I gave in.
# We use the funny ".mkcmd" key to read mkcmd's mind.  You dont want to
# know about that, really.  And we know about "-mkcmd" to read -A, -G.

require "getopt_key.m" "getopt.mc" "getopt.mi"
