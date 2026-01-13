/*@Version $Revision: 6.2 $@*/
/*@Header@*/
/* $Id: ckpass.c,v 6.2 2005/12/19 20:44:01 ksb Exp $
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <stdlib.h>
#include <pwd.h>

/*@Shell %echo ${MACHINE_H+"$MACHINE_H"}%@*/

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif


extern char *crypt();

/*@Explode @*/
/* Is this passwd a match for this user's passwd? 		(gregf/ksb)
 * look up passwd in shadow file if we have to, if we are
 * given a special epass try it first.
 * all the acSalt stuff is for dump crypts which fail on
 * salts with extra characters... sigh
 */

/*
 * MD5 code added by ajk for use on copernicus.physics.purdue.edu,
 * the console server for the Physics Computer Network at Purdue
 * University, which is quite happy to be running FreeBSD.
 * fixed by petef
 */
static int
MD5CheckPass(pcPass, pcWord)
	char *pcPass, *pcWord;
{
	register int iRet;
	const char acMagic[] = "$1$";
	register char *pcSalt, *pcCursor;

	/* grab the salt, up the '$' after acMagic. 8 chars max after magic */
	pcSalt = (char *)calloc(sizeof(char), strlen(pcPass) + 1);
	pcCursor = pcSalt = strdup(pcPass);
	pcCursor += strlen(acMagic);
	pcCursor = strstr(pcCursor, "$");
	*pcCursor = '\000';

	/* Don't hesitate; authenticate! */
	iRet = 0 == strcmp(pcPass, crypt(pcWord, pcSalt));
	free((void *)pcSalt);
	return iRet;
}

int
IsMD5Crypt(pcEPass)
char *pcEPass;
{
	const char acMagic[] = "$1$";

	if (0 == strncmp(pcEPass, acMagic, strlen(acMagic)))
		return 1;

	return 0;
}

int
CheckPass(pwd, pcEPass, pcWord)
struct passwd *pwd;
char *pcEPass, *pcWord;
{
	register char *pcCheck;
	auto char acSalt[4];
#if HAVE_SHADOW
	register struct spwd *spwd;
#endif

	pcCheck = (char *)0;

	if ((char *)0 != pcEPass && '\000' != pcEPass[0]) {
		pcCheck = pcEPass;
	}
#if HAVE_SHADOW
	else if ('x' == pwd->pw_passwd[0] && '\000' == pwd->pw_passwd[1]) {
		spwd = getspnam(pwd->pw_name);
		pcCheck = spwd->sp_pwdp;
	}
#endif
	else {
		pcCheck = pwd->pw_passwd;
	}

	if (IsMD5Crypt(pcCheck))
		return MD5CheckPass(pcCheck, pcWord);
	acSalt[0] = pcCheck[0];
	acSalt[1] = pcCheck[1];
	acSalt[2] = '\000';
	return 0 == strcmp(pcCheck, crypt(pcWord, acSalt));
}
