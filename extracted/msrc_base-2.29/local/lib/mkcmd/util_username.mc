/* $Id: util_username.mc,v 8.8 1999/06/25 16:41:26 ksb Exp $
 */

/* Get the user's password entry name, believe $USER or $LOGNAME	(ksb)
 * if they have the correct uid
 * we could look at utmp as well... -- ksb
 */
static char *
%%K<util_finduser>1v(pcOut)
char *pcOut;
{
	register char *pcEnv;
	register struct passwd *pwdMe;

	setpwent();
	if (((char *)0 != (pcEnv = getenv("USER")) || (char *)0 != (pcEnv = getenv("LOGNAME"))) &&
	    (struct passwd *)0 != (pwdMe = getpwnam(pcEnv)) &&
	    getuid() == pwdMe->pw_uid) {
		/* use the login $USER is set to, if it is our (real) uid */;
	} else if ((struct passwd *)0 == (pwdMe = getpwuid(getuid()))) {
%		fprintf(stderr, "%%s: getpwuid: %%d: %%s\n", %b, getuid(), %E)%;
		exit(1);
	}
	endpwent();
	return strcpy(pcOut, pwdMe->pw_name);
}
