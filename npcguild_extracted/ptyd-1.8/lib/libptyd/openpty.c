/*
 * This is the library interface to ptyd			    (mtr&ksb)
 *
 * Mike Rowan (mtr@mace.cc.purdue.edu)
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <syslog.h>
#include <signal.h>
#include <netdb.h>
#include <stdio.h>
#include <ctype.h>

#include "machine.h"
#include "openpty.h"
#include "ptyd.h"


static char *__pty_host;
static char *__pty_fmt;

#include "common.i"

/*
 * make the client port, connect it to the server			(ksb)
 * prototyped from screen
 */
static int
MakeClientPort(pcName)
char *pcName;
{
	register int s;
	struct sockaddr_un run;

	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		return -1;
	}
	run.sun_family = AF_UNIX;
	(void) strcpy(run.sun_path, pcName);
	if (connect(s, (struct sockaddr *)&run, strlen(pcName)+2) == -1) {
		(void) close(s);
		return -1;
	}
	return s;
}

/*
 * get a Joe pty bacause the daemon is not with us, sadly.		(ksb)
 */
static int
FallBack(pcSlave, pcMaster)
char *pcSlave, *pcMaster;
{
	auto int fd;
	auto char *pcTSlave, *pcTMaster;

	if (-1 == (fd = getpseudotty(& pcTSlave, & pcTMaster))) {
		return -1;
	}
	(void) strcpy(pcSlave, pcTSlave);
	(void) strcpy(pcMaster, pcTMaster);
	return fd;
}


#define MAXTRIES	10
/*
 * get be a pty pair, if fUtmp is non-zero build utmp entry too		(ksb)
 */
int
openpty(pcSlave, pcMaster, fBits, uid)
char *pcSlave, *pcMaster;
int fBits;
uid_t uid;
{
	auto int sPtyd, fd, iTries;
	auto char *pcPty;
	auto char acNumber[100];
	auto char acMsg[1025];

	if (-1 == (sPtyd = MakeClientPort(PTYD_PORT))) {
		if (0 != (fBits & OPTY_SECURE)) {
			return -1;
		}
		return FallBack(pcSlave, pcMaster);
	}

	if ((char *)0 == __pty_fmt)
		__pty_fmt = "%s";
	sprintf(acNumber, "%d", uid);

	/* Now that we are talking with ptyd, send our validation
	 * packet and read the pty/tty packet
	 */
	for (fd = -1, iTries = 0; iTries < MAXTRIES && fd == -1; ++iTries) {
		if (0 > Reply(sPtyd, PTYD_OPEN, acNumber)) {
			continue;
		}
		switch (Read(sPtyd, acMsg, sizeof(acMsg))){
		case -1:
			continue;
		default:
			break;
		}

		/* looks like he doesn't like us; but try again, try again...
		 */
		if ((char *)0 == (pcPty = IsCmd(acMsg, PTYD_TAKE))) {
			sleep(1);
			continue;
		}

		/* we might have one, chmod it and open it
		 */
		if (-1 == chmod(pcPty, SAFE_ACK)) {
			continue;
		}
		if (0 > (fd = open(pcPty, O_RDWR|O_NDELAY, 0600))) {
			continue;
		}
#if NEED_DELAY_HACK
		(void)fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK);
#endif

		/* yes, looking good now...
		 */
		strcpy(pcMaster, pcPty);
		WhichTty(pcSlave, pcPty);

		/* cannot chmod slave side?  get another....
		 */
		if (-1 == chmod(pcSlave, SAFE_ACK)) {
			close(fd);
			fd = -1;
			continue;
		}

		/* ask for utmp&wtmp entries for us, whoa.
		 * if we cannot finish this part it'll be OK.
		 */
		sprintf(acMsg, __pty_fmt, pcPty, (char *)0 != __pty_host ? __pty_host : "localhost");
		if (0 == (OPTY_UTMP&fBits)) {
			/* don't ask */;
		} else if (0 > Reply(sPtyd, PTYD_MKUTMP, acMsg)) {
			break;
		} else {
			(void) Read(sPtyd, acMsg, sizeof(acMsg));
			/* if ((char *)0 != IsCmd(acMsg, PTYD_OK)) ; */
		}

		if (0 == (OPTY_WTMP&fBits)) {
			/* don't ask */;
		} else if (0 > Reply(sPtyd, PTYD_LOGIN, pcMaster)) {
			break;
		} else {
			(void) Read(sPtyd, acMsg, sizeof(acMsg));
			/* if ((char *)0 != IsCmd(acMsg, PTYD_OK)) ; */
		}
	}

	/* drop connection to ptyd, he was a good friend
	 */
	close(sPtyd);

	if (-1 == fd) {
		if (0 != (fBits & OPTY_SECURE)) {
			return -1;
		}
		return FallBack(pcSlave, pcMaster);
	}
	return fd;
}

/*
 * same as open pty, but allow a remore machine to be in utmp record	(ksb)
 */
int
openrpty(pcSlave, pcMaster, fBits, uid, pcMach)
char *pcSlave, *pcMaster, *pcMach;
int fBits;
uid_t uid;
{
	int fd;

	__pty_host = pcMach;
	__pty_fmt = "%s@%s";
	fd = openpty(pcSlave, pcMaster, fBits, uid);
	__pty_host = __pty_fmt = (char *)0;

	return fd;
}

/*
 * close a pty pair for the user...					(ksb)
 */
int
closepty(pcSlave, pcMaster, fBits, fd)
char *pcSlave, *pcMaster;
int fBits, fd;
{
	auto int sPtyd;
	auto char acMsg[1025];

	if ('\000' == pcMaster[0] || '\000' == pcSlave[0]) {
		close(fd);
		return 0;
	}

	/* if we cannot get to the daemon, fake it
	 */
	if (-1 == (sPtyd = MakeClientPort(PTYD_PORT))) {
nohope:
		(void) chmod(pcMaster, PTYMODE);
		(void) chmod(pcSlave, TTYMODE);
		close(fd);
		return 0;
	}

	if (-1 == chmod(pcMaster, SAFE_DONE) || -1 == chmod(pcSlave, SAFE_DONE)) {
		close(fd);
		goto quit;
	}

	close(fd);

	if (0 != (fBits & OPTY_WTMP)) {
		if (-1 == Reply(sPtyd, PTYD_LOGOUT, pcMaster)) {
			close(sPtyd);
			goto nohope;
		}

		(void) Read(sPtyd, acMsg, 1024);
	}

	if (-1 == Reply(sPtyd, PTYD_CLOSE, pcMaster)) {
		close(sPtyd);
		goto nohope;
	}

	switch (Read(sPtyd, acMsg, 1024)) {
	case -1:
		close(sPtyd);
		goto nohope;
	case 0:
		goto quit;
	default:
		break;
	}
	if ((char *)0 == IsCmd(acMsg, PTYD_OK)) {
quit:
		close(sPtyd);
		return -1;
	}

	close(sPtyd);
	return 0;

}
