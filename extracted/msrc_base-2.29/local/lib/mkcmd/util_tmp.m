# $Id: util_tmp.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
#
# This template uses $TMPDIR to find a buffer directory.		(ksb)
# The default is /tmp unless $TMPDIR is set to a non-empty
# string.  We do *not* try to make sure the node given exists
# or is a directory.  User code should check with open/fopen/chdir/stat.
#
char* variable "pcTmpdir" {
	init getenv "TMPDIR"
	before 'if ((char *)0 == %n || \'\\000\' == *%n) {%n = "/tmp";}'
	help "name of temporary directory to use (default is /tmp)"
}
# after 'printf("%%s: tmpdir=\\"%%s\\"\\n", %b, pcTmpdir);'
