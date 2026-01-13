/*@Version $Revision: 6.0 $@*/
/* $Compile(*): ${cc-cc} ${cc_debug--g} -DTEST %f -o %F -lsocket -lnls
 */
/*@Header@*/
/*
 *	ident_client.c
 *
 * Identifies the remote user of a given connection.
 *
 * Written 940112 by Luke Mewburn <lm@rmit.edu.au>
 *
 * Copyright (C) 1994 by Luke Mewburn.
 * This code may be used freely by anyone as long as this copyright remains.
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

extern int errno;

#include "identd.h"

/* we look up our identd port, but fall back to this if
 * we can't find it in services
 */
#if !defined(IDENT_PORT)
#define IDENT_PORT	113
#endif


/*@Explode cli@*/
/* user interface to identd						(lm)
 * return 0 for OK, failure is non-zero
 * the passed down buffer contains the identity found, or is empty
 */
int
ident_client(peeraddr, ouraddr, pcIdent)
struct sockaddr_in
	peeraddr,	/* peer end, from getpeername(2)		*/
	ouraddr;	/* local end, from getsockname(2)		*/
char
	*pcIdent;	/* buffer to hold return ident string [1024]	*/
{
	auto struct sockaddr_in	authcon;
	auto int rport, lport;
	register struct servent *identserv;
	register int authfd, authlen;
	auto char acBuffer[512], reply_type[81], opsys_or_err[81];

	*pcIdent = '\000';
	if (-1 == (authfd = socket(AF_INET, SOCK_STREAM, 0))) {
		return errno;
	}
	/* INV: must close authfd before any return,
	 * escape to `oops' with errno != 0
	 */

	/* setup a connection to identd/authd on the remote host
	 */
	identserv = getservbyname("ident", "tcp");
	memset(&authcon, 0, sizeof(authcon));
	authcon.sin_family = AF_INET;
	authcon.sin_addr.s_addr = peeraddr.sin_addr.s_addr;
	authcon.sin_port = (struct servent *)0 != identserv ? identserv->s_port : ntohs(IDENT_PORT);

	authlen = sizeof(authcon);
	if (connect(authfd, (struct sockaddr *)&authcon, authlen) < 0) {
		goto oops;
	}

	/* ask the remote daemon about our client
	 */
	sprintf(acBuffer, "%d , %d\n", ntohs(peeraddr.sin_port), ntohs(ouraddr.sin_port));
	authlen = strlen(acBuffer);
	if (authlen != write(authfd, acBuffer, authlen)) {
		goto oops;
	}
	(void)shutdown(authfd, 1);

	/* wait for a reply -- we should select and timeout here ZZZ
	 */
	if (read(authfd, acBuffer, sizeof(acBuffer)-1) < 3) {
		goto oops;
	}

	/* unpack the results and return the good words to our caller
	 */
	authlen = sscanf(acBuffer, "%d , %d : %[^ \t\n\r:] : %[^ \t\n\r:] : %[^\n\r]", &lport, &rport, reply_type, opsys_or_err, pcIdent);
	if (authlen < 3) {
		errno = EINVAL;
		goto oops;
	}

	if (0 == strcasecmp(reply_type, "ERROR")) {
		errno = ENXIO;
		goto oops;
	}
	if (0 != strcasecmp(reply_type, "USERID")) {
		errno = ENOENT;
		goto oops;
	}
	(void)close(authfd);
	return 0;

oops:
	/* ... or bail out here ... */
	{
		register int iError;

		iError = errno;
		(void)close(authfd);
		*pcIdent = '\000';
		return iError;
	}
}

/*@Remove@*/
#if defined(TEST)

/*@Explode main@*/
#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif

#define I_AM_ON		7098

static struct sockaddr_in master_port, response_port;

char *progname = "identd-test";

/* test driver for the identd client module				(ksb)
 * bind to a port, wait for telnet connections,  then tell each who they are
 * [I really had to hold myself back here from putting a cmd line parser in!]
 */
int
main(argc, argv, envp)
int argc;
char **argv, **envp;
{
	auto struct sockaddr_in hisaddr, ouraddr;
	register int mfd, cfd, iRet;
	auto int true = 1, length, so;
	auto char acThem[1024];

	(void)memset((void *)&master_port, 0, sizeof(master_port));
	master_port.sin_family = AF_INET;
	*(u_long *)&master_port.sin_addr = INADDR_ANY;
	master_port.sin_port = htons(I_AM_ON);

	if (-1 == (mfd = socket(AF_INET, SOCK_STREAM, 0))) {
		fprintf(stderr, "%s: socket: %s\n", progname, strerror(errno));
		exit(1);
	}
#if defined(SO_REUSEADDR) && defined(SOL_SOCKET)
	if (-1 == setsockopt(mfd, SOL_SOCKET, SO_REUSEADDR, (char *)&true, sizeof(true))) {
		fprintf(stderr, "%s: setsockopt: %s\n", progname, strerror(errno));
		exit(1);
	}
#endif
	if (bind(mfd, (struct sockaddr *)&master_port, sizeof(master_port))<0) {
		fprintf(stderr, "%s: bind: %s\n", progname, strerror(errno));
		exit(1);
	}

	if (listen(mfd, SOMAXCONN) < 0) {
		fprintf(stderr, "%s: listen: %s\n", progname, strerror(errno));
		exit(1);
	}

	length = sizeof(ouraddr);
	if (getsockname(mfd, (struct sockaddr *)&ouraddr, &length) < 0) {
		fprintf(stderr, "%s: getsockname: %d: %s\n", progname, mfd, strerror(errno));

	}

	printf("%s: ready on port %d\n", progname, I_AM_ON);

	while (so = sizeof(response_port), -1 != (cfd = accept(mfd, (struct sockaddr *)&response_port, &so))) {
		length = sizeof(hisaddr);
		if (getpeername(cfd, (struct sockaddr *)&hisaddr, &length) < 0) {
			write(cfd, "can't get your addrees?\n", 24);
		} else if (0 != (iRet = ident_client(hisaddr, ouraddr, acThem))) {
			register char *pcError;

			pcError = strerror(iRet);
			write(cfd, pcError, strlen(pcError));
			write(cfd, "\n", 1);
		} else {
			write(cfd, acThem, strlen(acThem));
			write(cfd, "\n", 1);
		}
		(void)close(cfd);
	}

	exit(0);
}
/*@Remove@*/
#endif /* test driver */
