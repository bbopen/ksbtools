# $Id: std_noargs.m,v 8.8 1998/01/12 15:29:41 ksb Exp $
# use this template if you do not want any arguments, tty, true, false	(ksb)
# all might use this
list {
	param ""
	update "if (argc - %K<getopt_switches>1v > 0) {fprintf(stderr, \"%%s: too many arguments (%%s%%s)\\n\", %b, argv[%K<getopt_switches>1v], argc - %K<getopt_switches>1v > 1 ? \"...\" : \"\");exit(1);}"
}
