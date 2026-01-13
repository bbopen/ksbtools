#!mkcmd
# $Id: example_nettime.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
#	$Compile: mkcmd %f && gcc -o %F prog.c
# read the daytime port at a remote host

require "std_help.m" "client.m"

basename "nettime" ""

type "client" 'D' {
	local named "fdSock" "pcRemote"
	key 2 init {
		'"localhost:daytime/tcp"'
	}
}
exit {
	update "%n(%rDn);"
}
%c
static void
exit_func(fd)
int fd;
{
	register int cc;
	auto char acTime[32];

	if (-1 == fd) {
		exit(1);
	}
	while (0 < (cc = read(fd, acTime, sizeof(acTime)))) {
		write(1, acTime, cc);
	}
	exit(0);
}
