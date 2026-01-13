/*@Version $Revision: 3.6 $@*/
/*@Header@*/
/* $Id: fdrights.c,v 3.6 2009/10/16 15:41:17 ksb Exp $
 * $Compile: ${cc-cc} ${cc_debug--O} -DTEST %f -o %F
 * $Compile: ${cc-cc} ${cc_debug--O} -DTEST %f -o %F -lsocket
 * $Test: date > /tmp/test && ./%F
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/uio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "fdrights.h"

#if !defined(USE_STRINGS)
#define USE_STRINGS	(defined(bsd))
#endif

/*@Shell %echo ${MACHINE_H+"$MACHINE_H"}%@*/
/*@Remove@*/
#include "machine.h"
/*@Explode send@*/
/* Send an array of descriptors through a unix domain socket.		(bj)
 * return -1 on error, 0 on success
 */
int
RightsSendFd(s, piFds, iFdCount, pcText)
int s;
int *piFds;
unsigned iFdCount;
char *pcText;
{
	int error;
	struct msghdr msg;
	struct iovec iov;
#if HAVE_SCM_RIGHTS
	struct cmsghdr *pcmsg;
	int iLen;

	iLen = sizeof(struct cmsghdr) + sizeof(int)*iFdCount;
	if ((struct cmsghdr *)0 == (pcmsg = (struct cmsghdr *) malloc(iLen))) {
		errno = ENOMEM;
		return -1;
	}
	pcmsg->cmsg_len = iLen;
	pcmsg->cmsg_level = SOL_SOCKET;
	pcmsg->cmsg_type = SCM_RIGHTS;
#if USE_STRINGS
	bcopy(piFds, pcmsg+1, sizeof(int)*iFdCount);
#else
	memcpy(pcmsg+1, piFds, sizeof(int)*iFdCount);
#endif
	msg.msg_control = (caddr_t) pcmsg;
	msg.msg_controllen = pcmsg->cmsg_len;
#else
	msg.msg_accrights = (caddr_t) piFds;
	msg.msg_accrightslen = sizeof(int) * iFdCount;
#endif /* HAVE_SCM_RIGHTS */

	/* no name (assume socket is connected */
	if ((char *)0 == pcText) {
		pcText = "ok";
	}
	msg.msg_name = (char *)0;
	msg.msg_namelen = 0;
	/* some sync data data and our file descriptors */
	iov.iov_base = pcText;
	iov.iov_len = strlen(pcText)+1;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	error = sendmsg(s, &msg, 0);

#if HAVE_SCM_RIGHTS
	{
		int e = errno;
		free((char *) pcmsg);
		errno = e;
	}
#endif /* HAVE_SCM_RIGHTS */

	return error;
}

/*@Explode recv@*/
char acRightsRecvBuf[RIGHTS_TAG_LINE];

/* Receive an array of descriptors through a unix domain socket		(bj)
 * return -1 on error, 0 on success
 */
int
RightsRecvFd(s, piFds, iFdCount, pcText, piTextLen)
int s;
int *piFds;
unsigned iFdCount;
char *pcText;
size_t *piTextLen;
{
	int error;
	struct msghdr msg;
	struct iovec iov;
#if HAVE_SCM_RIGHTS
	struct cmsghdr *pcmsg;
	int iLen;

	iLen = sizeof(struct cmsghdr) + sizeof(int)*iFdCount;
	if ((struct cmsghdr *)0 == (pcmsg = (struct cmsghdr *) malloc(iLen))) {
		errno = ENOMEM;
		return -1;
	}
	pcmsg->cmsg_len = iLen;
	pcmsg->cmsg_level = SOL_SOCKET;
	pcmsg->cmsg_type = SCM_RIGHTS;
	msg.msg_control = (caddr_t) pcmsg;
	msg.msg_controllen = pcmsg->cmsg_len;
#else
	msg.msg_accrights = (caddr_t) piFds;
	msg.msg_accrightslen = sizeof(int) * iFdCount;
#endif /* HAVE_SCM_RIGHTS */

	/* no name (assume socket is connected */
	msg.msg_name = (char *)0;
	msg.msg_namelen = 0;
	/* some sync data data and our file descriptors */
	if ((char *)0 != pcText) {
		iov.iov_base = pcText;
		iov.iov_len = (size_t *)0 == piTextLen ? 0 : *piTextLen;
	} else {
		iov.iov_base = acRightsRecvBuf;
		iov.iov_len = sizeof(acRightsRecvBuf);
	}
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	error = recvmsg(s, &msg, 0);

#if HAVE_SCM_RIGHTS
	{
		int e = errno;
#if USE_STRINGS
		bcopy(pcmsg+1, piFds, sizeof(int)*iFdCount);
#else
		memcpy(piFds, pcmsg+1, sizeof(int)*iFdCount);
#endif
		free((char *) pcmsg);
		errno = e;
	}
#endif /* HAVE_SCM_RIGHTS */

	return error;
}

/*@Explode wrapper@*/
/* We got back not a - (fail) or + (just use the open socket) from	(ksb)
 * a call to divConnect, so we accept access rights and use them (@).
 * This is the client turbo for wrapw, and you must have it to allow
 * all the magics wrapw provides to a wrapper.
 */
int
RightsWrap(int fdWrapw, char *pcCookie, void *pvData, char *pcType)
{
	auto int iLen, fdPriv;

	iLen = sizeof(acRightsRecvBuf);
	if (-1 == RightsRecvFd(fdWrapw, &fdPriv, 1, acRightsRecvBuf, &iLen)) {
		return -1;
	}
	close(fdWrapw);
	return fdPriv;
}

/*@Explode ropen@*/

static int iRopenFd = -1;

/* fork and turn the child into an "ropen() slave", so the parent 	(bj)
 * can use ropen() to open files in the other process context.
 * returns the pid of the child or -1 on error.
 */
int
RopenSplit(func)
void (*func)();
{
	register int iPid, s;
	int fd;
	long iLen;

	if (-1 != iRopenFd) {
		close(iRopenFd);
	}
	{
		int aiS[2];

		if (-1 == socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, aiS)) {
			return -1;
		}
		iRopenFd = aiS[0];
		s = aiS[1];
	}

	if (0 != (iPid = fork())) {
		int e = errno;

		close(s);
		if (-1 == iPid) {
			close(iRopenFd);
			iRopenFd = -1;
		}
		errno = e;
		return iPid;
	}
	/* this is the slave side.  ropen() sends us a length followed
	 * by mode and perms and the "length" bytes of filename.  we
	 * open it and send success info and the open file descriptor back.
	 */
	close(iRopenFd);
	iRopenFd = -1;
	if (func) {
		(*func)();
	}
	while (sizeof(long) == read(s, &iLen, sizeof(long))) {
		long iMode, iPerm;
		char acFile[MAXPATHLEN];

		if (sizeof(long) != read(s, &iMode, sizeof(long))) {
			break;
		}
		if (sizeof(long) != read(s, &iPerm, sizeof(long))) {
			break;
		}
		if (iLen >= MAXPATHLEN || iLen != read(s, acFile, iLen)) {
			break;
		}
		if (-1 != (fd = open(acFile, iMode, iPerm))) {
			errno = 0;
		}
		write(s, &errno, sizeof(errno));	/* 0 is success */
		if (-1 == fd) {
			continue;
		}
		RightsSendFd(s, &fd, 1, "ok");
		close(fd);
	}
	close(s);
	exit(0);
	/*NOTREACHED*/
}

/* emulate the open() call by calling through the slave in another	(bj)
 * process context
 */
int
ropen(pcFile, iMode, iPerm)
char *pcFile;
int iMode;
int iPerm;
{
	int fd;
	long iLen;

	if (-1 == iRopenFd) {
		errno = ESRCH;
		return -1;
	}
	iLen = strlen(pcFile) + 1;
	write(iRopenFd, &iLen, sizeof(long));
	write(iRopenFd, &iMode, sizeof(long));
	write(iRopenFd, &iPerm, sizeof(long));
	write(iRopenFd, pcFile, iLen);
	if (sizeof(errno) != read(iRopenFd, &errno, sizeof(errno)) || 0 != errno) {
		return -1;
	}
	if (-1 == RightsRecvFd(iRopenFd, &fd, 1, (char *)0, (size_t *)0)) {
		return -1;
	}
	return fd;
}

/*
 * TEST DRIVER
 */
/*@Remove@*/
#ifdef TEST
/*@Explode test@*/
#include <stdio.h>

extern int errno;
#if !defined(HAVE_STRERROR)
#define HAVE_STRERROR 1
#endif
#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me)	(sys_errlist[Me])
#endif

static char *progname;
static char *pcInDir = "/tmp";

/* this function is what separates the caller from the `remote' end	(bj)
 * it could:
 *	chdir, chroot, setuid, setgid, ....
 * anything that makes open() do something else.  This examples chdirs.
 */
static void
cdfunc()
{
	if (-1 == chdir(pcInDir)) {
		fprintf(stderr, "%s: chdir: %s: %s\n", progname, pcInDir, strerror(errno));
		exit(1);
	}
}


/* driver to open a file in /tmp and show that ropen works		(bj)
 */
int
main(argc, argv)
int argc;
char **argv;
{
	auto int aiSocks[2];
	auto int aiFd[1];
	auto int acLine[80];
	auto char acTag[64];
	auto size_t iTag;
	register int fd, cc;
	register int count;
	register char *pcToFile;

	progname = argv[0];
	switch (argc) {
	case 3:
		pcInDir = argv[2];
		/* fall through */
	case 2:
		pcToFile = argv[1];
		break;
	case 1:
		pcToFile = "test";
		break;
	default:
		fprintf(stderr, "%s: usage [file] [dir]\n", progname);
		fprintf(stderr, "file  a file in dir to open (default test)\n");
		fprintf(stderr, "dir   chdir to a directory first (default /tmp)\n");
		exit(0);
	}
	if (-1 == socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, aiSocks)) {
		perror("socketpair");
		exit(1);
	}
	printf("%s: sending stdout (fd #1) to myself\n", progname);
	fflush(stdout);
	aiFd[0] = 1;
	if (-1 == (count = RightsSendFd(aiSocks[0], aiFd, 1, "stdout"))) {
		perror("RightsSendFd");
		exit(1);
	}
	aiFd[0] = -1;
	iTag = sizeof(acTag);
	if (-1 == (count = RightsRecvFd(aiSocks[1], aiFd, 1, acTag, &iTag))) {
		perror("RightsRecvFd");
		exit(1);
	}
	printf("%s: got %s (fd #%d) ... testing it\n", progname, acTag, aiFd[0]);
	fflush(stdout);
	write(aiFd[0], "hello world\n", 12);
	close(aiFd[0]);

	printf("%s: testing ropen [in %s] of file %s\n", progname, pcInDir, pcToFile);
	fflush(stdout);
	RopenSplit(cdfunc);
	if (-1 == (fd = ropen(pcToFile, 0, 444))) {
		fprintf(stderr, "%s: ropen: [in %s] %s: %s\n", progname, pcInDir, pcToFile, strerror(errno));
		exit(1);
	}
	printf("%s: got fd #%d ... testing it\n", progname, fd);
	fflush(stdout);

	if (-1 != fd && 0 < (cc = read(fd, acLine, sizeof(acLine)))) {
		write(1, acLine, cc);
	}

	printf("%s: done.\n", progname);
	exit(0);
}
/*@Remove@*/
#endif
