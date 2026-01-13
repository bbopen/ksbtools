#!mkcmd
# $Id: example_rc.m,v 1.3 1999/06/12 00:43:58 ksb Exp $
# two example rc files, an option bound and a fixed-name one

require "std_help.m" "rc.m"

basename "rc" ""

type "rc" 'f' {
	named "fpConfig" "pcConfig"
	init '"think"'
	help "an rc file you can set the name on, default %qi"
}

type "rc" named "fpKink" "pcKink" {
	help "and rc file that is named for the program name"
# if we wanted to look in only one place
%c
static char *apcLook[] = { "/var/kink" } ;
%%
	key 2 {
		3 "apcLook"
	}
}

cleanup "ShowEm();"

%c
static void
ShowEm()
{
	if ((FILE *)0 != fpKink) {
		printf("%s: found a kink: %s\n", progname, pcKink);
	}
	if ((FILE *)0 != fpConfig) {
		printf("%s: found a config: %s\n", progname, pcConfig);
	}
}
%%
