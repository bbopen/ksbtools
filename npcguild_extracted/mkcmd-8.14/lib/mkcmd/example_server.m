#!mkcmd
# $Id: example_server.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
# provide a unique number service
#
# Explain to mk how to build this program:
#	$Compile: mkcmd %f && %b prog.c
comment "%c%kCompile(sun): ${cc-cc} %%f -lnls -lsocket"
comment "%c%kCompile(*): ${cc-cc} %%f"

require "std_help.m" "server.m"

basename "iota" ""

type "server" 'L' {
	local named "fdSock" "pcRemote"
	key 2 initialize {
		'"*:10305/tcp"'
	}
}
exit {
	update "%n(%rLn);"
}

%c
/* implement the primative service					(ksb)
 */
static void
exit_func(fdServer)
int fdServer;
{
	register int fdClient;
	register int iUniq;
	auto char acAnswer[32];
	auto struct sockaddr_in SAClient;
	auto int iSize;

	if (-1 == fdServer) {
		exit(1);
	}
	iUniq = 0;
	for (;;) {
		iSize = sizeof(SAClient);
		if (-1 == (fdClient = accept(fdServer, (struct sockaddr *)&SAClient, &iSize))) {
			if (errno == EINTR)
				continue;
			fprintf(stderr, "%s: accept: %s\n", progname, strerror(errno));
			exit(1);
		}
		sprintf(acAnswer, "%d\r\n", ++iUniq);
		write(fdClient, acAnswer, strlen(acAnswer));
		sleep(1);
		close(fdClient);
	}
	exit(0);
}
