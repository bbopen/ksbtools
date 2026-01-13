/* $Id: machine.c,v 2.4 2006/12/30 03:02:27 ksb Exp $
 */
#include <stdio.h>
#include <sysexits.h>
#include <string.h>
#include <grp.h>
#include <pwd.h>

#include "machine.h"

extern char *progname;
extern char *pcGuilty;

#if !HAS_GETPW_EXTERNS
extern struct passwd *getpwuid(), *getpwname(), *getpwent();
extern struct group *getgruid(), *getgrname(), *getgrent();
#endif

#if !HAVE_STRDUP

char *strdup (s)
const char *s;
{
	register char *result;

	result = (char *) malloc (strlen(s) + 1);
	strcpy(result, s);
	return result;
}

#endif /* !HAVE_STRDUP */

#if !HAS_GETGROUPS
int
getgroups(iMax, piVec)
int iMax, *piVec;
{
	register struct passwd *pwd;
	register struct group *grp;
	auto char acMe[32];
	register int iGid, iLen;

	setpwent();
	if ((char *)0 != pcGuilty && (struct passwd *)0 != (pwd = getpwnam(pcGuilty)) && pwd->pw_uid == getuid()) {
		(void)strcpy(acMe, pwd->pw_name);
	} else if ((struct passwd *)0 != (pwd = getpwuid(getuid())) && pwd->pw_uid == getuid()) {
		(void)strcpy(acMe, pwd->pw_name);
	} else {
		fprintf(stderr, "%s: getgroups: %d: no such user\n", progname, getuid());
		exit(EX_NOUSER);
	}
	endpwent();

	setgrent();
	iLen = 0;
	while ((struct group *)0 != (grp = getgrent()) && iLen < iMax) {
		register char **ppcWho;
		if (iGid == grp->gr_gid)
			continue;
		for (ppcWho = grp->gr_mem; (char *)0 != *ppcWho; ++ppcWho) {
			if (0 == strcmp(acMe, *ppcWho)) {
				piVec[iLen++] = grp->gr_gid;
				break;
			}
		}
	}
	endgrent();

	return iLen;
}
#endif
