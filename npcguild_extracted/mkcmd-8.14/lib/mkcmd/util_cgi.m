#!mkcmd
# $Id: util_cgi.m,v 8.7 1997/10/11 14:51:10 ksb Beta $
# Turn CGI params into an argv
# Bugs:
#  In "a&&b" the two and signs are treated as a single argument sep,
# just as if they were spaces.
#
#  We ignore most of the environment the web server set, here are the
# variables we know might be set:
#	SERVER_SOFTWARE SERVER_NAME GATEWAY_INTERFACE SERVER_PROTOCOL
#	SERVER_PORT REQUEST_METHOD HTTP_ACCEPT PATH_INFO PATH_TRANSLATED
#	SCRIPT_NAME QUERY_STRING REMOTE_HOST REMOTE_ADDR REMOTE_USER
#	AUTH_TYPE CONTENT_TYPE CONTENT_LENGTH
# We only look at $PATH_INFO is argv doesn't have any data for us.

from '<stdlib.h>'
from '<stdio.h>'
from '<ctype.h>'

require "util_cgi.mc"

# need u_envopt code
getenv

init 1 "(void)cgi_args(& argc, & argv);"

key "cgi_args" 1 init {
	"PATH_INFO"
}
