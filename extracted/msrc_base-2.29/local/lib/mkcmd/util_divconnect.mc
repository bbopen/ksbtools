/* $Id: util_divconnect.mc,v 1.3 2008/09/01 17:35:13 ksb Exp $
 */

/* Open a client socket, but hook in the wrapw magic too.		(ksb)
 * Make the client port, connect it to the enclosing diversion.
 * Wrapw waits for the non-existant path tail to return the service rights,
 * so send the cookie part and wait for the three reply cases:
 *	-	we failed to find the instance, return -1
 *	+	just hand this one back (I'll proxy),
 *    <else>	accept access rights via the indirect pointer
 * Then protect it from being passed to a child, and hook into any
 * initial negotiation the client needs.
 *
 * If we must recurse use the divNextLevel functor to find the parameter
 * to pass back to this function, when not set pass (void *)0.
 */
int
%%K<divConnect>/ef
{
	register int s, iTry, iCc;
	register char *pcName, *pcCookie, *pcSlash;
	auto struct sockaddr_un run;
	auto struct stat stExist;
	auto char acAnswer[4];

	s = -1;
	if (-1 == stat(pcSocket, & stExist)) {
		if (ENOTDIR != errno) {
			return -1;
		}
		pcName = strdup(pcSocket);
		if ((char *)0 == (pcCookie = strrchr(pcName, '/'))) {
			errno = EPIPE;
			free((void *)pcName);
			return -1;
		}
		/* Handle any user built $SOCKET///cookie
		 */
		pcSlash = pcCookie++;
		while (pcSlash > pcName && '/' == pcSlash[-1]) {
			--pcSlash;
		}
		*pcSlash = '\000';
		iCc = strlen(pcCookie)+1;
%		if (-1 == (s = %K<divConnect>1ev(pcName, pfiIndir, (void *(*)(void *))0 == divNextLevel ? (void *)0 : (*divNextLevel)(pvLevel)))) %{
			return -1;
		} else if (iCc != write(s, pcCookie, iCc)) {
			close(s);
			return -1;
		} else if (1 != read(s, acAnswer, 1)) {
			close(s);
			return -1;
		} else if ('-' == acAnswer[0]) {	/* failed to find */
			close(s);
			return -1;
		} else if ('+' == acAnswer[0]) {	/* use this channel */
			/* ok, just take it */
		} else if ((int (*)())0 == pfiIndir) {	/* no accept rights */
			close(s);
			errno = ENOSYS;
			return -1;
		} else /* @ or something else in the future */ {
			/* Indirect case let them do the protocol and
			 * they have to close s if they don't return it.
			 */
			acAnswer[1] = '\000';
			s = (*pfiIndir)(s, pcCookie, pvLevel, acAnswer);
		}
		free((void *)pcName);
	}
	for (iTry = 0; -1 == s && iTry < 10; ++iTry) {
		if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
			return -1;
		}
		run.sun_family = AF_UNIX;
		(void)strcpy(run.sun_path, pcSocket);
		if (-1 != connect(s, (struct sockaddr *)&run, EMU_SOCKUN_LEN(pcSocket))) {
			break;
		}
		if (errno != ECONNREFUSED) {
			close(s);
			s = -1;
			break;
		}
		close(s);
		s = -1;
		sleep(iTry); /* wait up to 55 seconds */
	}
	if (-1 != s) {
		(void)fcntl(s, F_SETFD, 1);
	}
	if ((int (*)(int, void *))0 != divConnHook) {
		return (*divConnHook)(s, pvLevel);
	}
	return s;
}
