/*@Version $Revision: 1.7 $@*/
/*@Header@*/
/* $Id: cred.c,v 1.7 2009/10/16 15:41:17 ksb Exp $
 *
 * Find a way to credential a client.  We can be explode'd into a client
 * only side, or a server side.  Built on the same host we should always
 * be compatible with ourself (one would hope).			(csg, ksb)
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cred.h"

/*@Shell %echo ${MACHINE_H+"$MACHINE_H"}%@*/

/* local configuration
 */
#if !defined(CRED_TOKEN)
#define CRED_TOKEN		"CrEd"
#endif

#if !defined(CRED_TOKEN_LEN)
#define CRED_TOKEN_LEN		sizeof(CRED_TOKEN)
#endif

#if !defined(CRED_SPEW)
#define CRED_SPEW	0	/* be silnet by default */
#endif

#if CRED_SPEW
/*@Insert "extern char *proganme;"@*/
#endif

/*@Remove@*/
#ifdef TEST
/*@Explode test@*/
/* $Compile: gcc -DTEST -Wall -o%F %f
 */
static char *progname =
	"credtest";
/*@Remove@*/
#endif /* TEST */

/*@Explode dump@*/
#include <grp.h>
#include <pwd.h>

/* Print a struct cmsgcred in human-readable form			(csg)
 */
void
DumpCred(CRED_TYPE *pCred)
{
	register struct passwd *pPW;
	register struct group *pGR;
#if defined(SCM_CREDS)
	register int i;
#endif

#if defined(SCM_CREDS)
	fprintf(stderr, "PID=%d ", pCred->cmcred_pid);

	pPW = getpwuid(pCred->cmcred_uid);
	fprintf(stderr, "UID=%s:%d ", (pPW != NULL) ? pPW->pw_name : "*",
		pCred->cmcred_uid);

	pPW = getpwuid(pCred->cmcred_euid);
	fprintf(stderr, "EUID=%s:%d ", (pPW != NULL) ? pPW->pw_name : "*",
		pCred->cmcred_euid);

	pGR = getgrgid(pCred->cmcred_gid);
	fprintf(stderr, "GID=%s:%d ", (pGR != NULL) ? pGR->gr_name : "*",
		pCred->cmcred_gid);

	pGR = getgrgid(pCred->cmcred_groups[0]);
	fprintf(stderr, "EGID=%s:%d\nGROUPS=", (pGR != NULL) ? pGR->gr_name : "*",
		pCred->cmcred_groups[0]);

	for (i=1; i < pCred->cmcred_ngroups; i++) {
		pGR = getgrgid(pCred->cmcred_groups[i]);
		fprintf(stderr, "%s:%d ", (pGR != NULL) ? pGR->gr_name : "*",
			pCred->cmcred_groups[i]);
	}
#else
	fprintf(stderr, "PID=%d ", pCred->pid);
	pPW = getpwuid(pCred->uid);
	fprintf(stderr, "EUID=%s:", (pPW != NULL) ? pPW->pw_name : "*");
	pGR = getgrgid(pCred->gid);
	fprintf(stderr, "EGID=%s\n", (pGR != NULL) ? pGR->gr_name : "*");
#endif
	fprintf(stderr, "\n");
}

/*@Explode send@*/
/* Send our credentials to our peer on an AF_UNIX socket.		(csg)
 * returns -1 on error, 0 on success.
 */
int
SendCred(int s)
{
	register int iRet = 0;
	register int iErr, iLen;
	register struct cmsghdr *pCM;
	auto struct msghdr msg;
	auto struct iovec iov;

#if defined(LOCAL_CREDS)
	{
	auto int fOne;
	fOne = 1;
	(void)setsockopt(s, 0, LOCAL_CREDS, (void *)&fOne, sizeof(fOne));
	}
#endif
	iLen = sizeof(struct cmsghdr) + sizeof(CRED_TYPE);
	if ((struct cmsghdr *)0 == (pCM = (struct cmsghdr *)calloc(1, iLen))) {
		errno = ENOMEM;
		return -1;
	}

	pCM->cmsg_len = iLen;
	pCM->cmsg_level = SOL_SOCKET;
	pCM->cmsg_type = CRED_SCM_MSG;

	/* We must send some in-band data.  We look for this data on the
	 * peer host, to make sure we're in sync.
	 */
	bzero(&iov, sizeof(struct iovec));
	iov.iov_base = CRED_TOKEN;
	iov.iov_len = CRED_TOKEN_LEN;

	bzero(&msg, sizeof(struct msghdr));
	msg.msg_name = (caddr_t)0;
	msg.msg_namelen = 0;
	msg.msg_control = (caddr_t)pCM;
	msg.msg_controllen = pCM->cmsg_len;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;

	if (CRED_TOKEN_LEN != (iRet = sendmsg(s, &msg, 0))) {

#if CRED_SPEW
		fprintf(stderr, "%s: sendmsg: %d: %s\n", progname, s, strerror(errno));
#endif
	}
	iErr = errno;
	free((void *)pCM);
	errno = iErr;
	return iRet;
}

/*@Explode recv@*/
/* Receive credentials from our peer on an AF_UNIX socket.		(csg)
 * returns -1 on error, 0 on success.
 */
int
RecvCred(int s, CRED_TYPE *pCred)
{
	register int iRet = 0;
	register int iErr, iLen;
	register struct cmsghdr *pCM;
	auto struct msghdr msg;
	auto struct iovec iov;
	auto char acBuf[CRED_TOKEN_LEN];
	auto char acWrong[1024];

#if defined(LOCAL_PEERCRED)
	{
	auto int fOne;
	fOne = 1;
	(void)setsockopt(s, 0, LOCAL_PEERCRED, (void *)&fOne, sizeof(fOne));
	}
#endif
	iLen = sizeof(struct cmsghdr) + sizeof(CRED_TYPE);
	if ((struct cmsghdr *)0 == (pCM = (struct cmsghdr *)calloc(1, iLen))) {
		errno = ENOMEM;
		return -1;
	}

	pCM->cmsg_len = iLen;
	pCM->cmsg_level = SOL_SOCKET;
	pCM->cmsg_type = CRED_SCM_MSG;

	bzero(&iov, sizeof(struct iovec));
	iov.iov_base = acBuf;
	iov.iov_len = sizeof(acBuf);

	bzero(&msg, sizeof(struct msghdr));
	bzero(acWrong, sizeof(acWrong));
	msg.msg_name = acWrong;
	msg.msg_namelen = sizeof(acWrong);
	msg.msg_control = (caddr_t)pCM;
	msg.msg_controllen = sizeof(CRED_TYPE);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_flags = MSG_WAITALL;

	if (-1 == (iRet = recvmsg(s, &msg, 0))) {
#if CRED_SPEW
		fprintf(stderr, "%s: recvmsg: %d: %s\n", progname, s, strerror(errno));
#endif
		return -1;
	}
	if (CRED_TOKEN_LEN != iRet) {
#if CRED_SPEW
		fprintf(stderr, "%s: message is the wrong size (%d != %d)\n", progname, iLen, iRet);
#endif
		errno = EFBIG;
		return -1;
	}
	if (0 != msg.msg_namelen) {
#if CRED_SPEW
		fprintf(stderr, "%s: unexpected name of %d bytes\"%s\"\n", progname, msg.msg_namelen, acWrong);
#endif
		errno = E2BIG;
		return -1;
	}
	if (iLen < msg.msg_controllen) {
#if CRED_SPEW
		fprintf(stderr, "%s: payload is too big (%d != %d)\n", progname, iLen, msg.msg_controllen);
#endif
		errno = E2BIG;
		return -1;
	}

	/* Make sure we're in sync with SendCred() */
	if (0 != strncmp(acBuf, CRED_TOKEN, CRED_TOKEN_LEN)) {
#if CRED_SPEW
		fprintf(stderr, "%s != %s\n", acBuf, CRED_TOKEN);
#endif
		return -2;
	}

	iErr = errno;
	(void)memcpy(pCred, CMSG_DATA(pCM), sizeof(CRED_TYPE));
	free((void *)pCM);
	errno = iErr;
	return iRet;
}

/*@Remove@*/
#ifdef TEST

/*@Append test@*/
/*@Insert ~#include "cred.h"~@*/

/* A simple test driver for the cred functions above.		(csg,ksb)
 */
int
main(argc, argv)
int argc;
char **argv;
{
	register int iErr, iKid;
	auto int aiSV[2];
	auto CRED_TYPE Cred;

	if (-1 == socketpair(AF_UNIX, SOCK_STREAM, 0, aiSV)) {
		fprintf(stderr, "%s: socketpair: %s\n", progname, strerror(errno));
		exit(1);
	}

	if (-1 == (iKid = fork())) {
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		exit(1);
	}

	/* the kid sends her creds back to the original
	 */
	if (0 == iKid) {
		close(aiSV[0]);
		if (-1 == (iErr = SendCred(aiSV[1]))) {
			fprintf(stderr, "%s: SendCred: %s\n", progname, strerror(errno));
		}
		close(aiSV[1]);
		exit(0);
	}

	/* The original waits for the word
	 */
	close(aiSV[1]);
	if (-1 == (iErr = RecvCred(aiSV[0], &Cred))) {
		fprintf(stderr, "%s: RecvCred: %s\n", progname, strerror(errno));
	}
	/* we sleep to avoid mixing output with the other process
	 */
	sleep(1);

	/* Now sanity check what we got
	 */
#if defined(SCM_CREDS)
	if (iKid != Cred.cmcred_pid) {
		fprintf(stderr, "PID: %d != %d\n", iKid, Cred.cmcred_pid);
	}
	if (getuid() != Cred.cmcred_uid) {
		fprintf(stderr, "UID: %d != %d\n", getuid(), Cred.cmcred_uid);
	}
	if (geteuid() != Cred.cmcred_euid) {
		fprintf(stderr, "EUID: %d != %d\n", getuid(), Cred.cmcred_euid);
	}
	if (getgid() != Cred.cmcred_gid) {
		fprintf(stderr, " GID: %d != %d\n", getgid(), Cred.cmcred_gid);
	}
	if (getegid() != Cred.cmcred_groups[0]) {
		fprintf(stderr, "EGID: %d != %d\n", getegid(), Cred.cmcred_groups[0]);
	}

	if (Cred.cmcred_ngroups != (iErr = getgroups(0, (gid_t *)0))) {
		fprintf(stderr, "NGROUPS: %d != %d\n", Cred.cmcred_ngroups, iErr);
	}
	/* XXX: Check the whole list of groups?, they could be permuted
	 */
#else
	if (iKid != Cred.pid) {
		fprintf(stderr, "PID: %d != %d\n", iKid, Cred.pid);
	}
	if (geteuid() != Cred.uid) {
		fprintf(stderr, "EUID: %d != %d\n", getuid(), Cred.uid);
	}
	if (getegid() != Cred.gid) {
		fprintf(stderr, "EGID: %d != %d\n", getegid(), Cred.gid);
	}
#endif

	DumpCred(&Cred);
	close(aiSV[0]);

	exit(0);
}

/*@Remove@*/
#endif /* TEST */
