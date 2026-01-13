/* $Id: util_home.mc,v 8.8 1998/09/16 13:34:24 ksb Exp $
 */

/* find a home directory for ~usr style interface			(ksb)
 *	do ~login and ~
 * Input login names are terminated by '\000' or '/' or ':',
 * anything else you can do.  pcDest should have about MAXPATHLEN
 * characters available, more or less.
 *
 * We return the pointer after the consumed text, or (char *)0 for fail,
 * if the destination buffer is empty we could not find $HOME/getpwuid,
 * else we could not find the login name in the buffer.
 */
char *
%%K<util_home>1v(pcDest, pcWho)
char *pcDest, *pcWho;
{
	extern char *getenv();
	register char *pcHome;
	register struct passwd *pwd;
	register char *pcTmp;

	pcHome = getenv("HOME");
	if ((char *)0 == pcHome) {
		pwd = getpwuid(getuid());
		/* only good for a short time... long enough
		 */
		if ((struct passwd *)0 != pwd) {
			pcHome = pwd->pw_dir;
		}
	}
	if ((char *)0 == pcWho || '/' == *pcWho || ':' == *pcWho || '\000' == *pcWho) {
		if ((char *)0 == pcHome) {
			*pcDest = '\000';
			return (char *)0;
		}
		(void)strcpy(pcDest, pcHome);
		return (char *)0 == pcWho || '\000' == *pcWho ? "" : pcWho;
	}

	pcTmp = pcDest;
	while ('/' != *pcWho && ':' != *pcWho && '\000' != *pcWho) {
		*pcTmp++ = *pcWho++;
	}
	*pcTmp = '\000';

	if ((struct passwd *)0 == (pwd = getpwnam(pcDest))) {
		return (char *)0;
	}
	(void)strcpy(pcDest, pwd->pw_dir);
	return pcWho;
}
