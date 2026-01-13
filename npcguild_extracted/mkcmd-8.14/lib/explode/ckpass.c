/*@Version $Revision: 6.0 $@*/
/*@Header@*/
/* $Id: ckpass.c,v 6.0 2000/07/30 15:57:11 ksb Exp $
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
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
#if USE_MD5
/*
 * MD5 code added by ajk for use on copernicus.physics.purdue.edu,
 * the console server for the Physics Computer Network at Purdue
 * University, which is quite happy to be running FreeBSD.
 */
static int
MD5CheckPass(pcPass, pcWord)
	char *pcPass, *pcWord;
{
	const char acMagic[] = "$1$";
	auto char acSalt[8+1];
	register char *pcSalt;
	register size_t iMagic;
	register char *pcDollar;

	iMagic = strlen(acMagic);
	pcSalt = pcPass;
	/* skip a preceding magic string */
	if (0 == strncmp(pcSalt, acMagic, iMagic))
		pcSalt += (int) iMagic;

	/* grab the salt (to the next '$', or 8 chars max.) */
	(void) strncpy(acSalt, pcSalt, 8);
	acSalt[8] = '\000';
	pcDollar = strchr(acSalt, '$');
	if (pcDollar)
		*pcDollar = '\000';

	/* Don't hesitate; authenticate! */
	return 0 == strcmp(pcPass, crypt(pcWord, acSalt));
}

int
CheckPass(pwd, pcEPass, pcWord)
struct passwd *pwd;
char *pcEPass, *pcWord;
{
	if (pcEPass && '\000' != pcEPass[0])
		if (MD5CheckPass(pcEPass, pcWord))
			return 1;
	return MD5CheckPass(pwd->pw_passwd, pcWord);
}
#else /* USE_MD5 */
int
CheckPass(pwd, pcEPass, pcWord)
struct passwd *pwd;
char *pcEPass, *pcWord;
{
	auto char acSalt[4];
#if HAVE_SHADOW
	register struct spwd *spwd;
#endif

	acSalt[2] = '\000';
	if ((char *)0 != pcEPass && '\000' != pcEPass[0]) {
		acSalt[0] = pcEPass[0];
		acSalt[1] = pcEPass[1];
		if (0 == strcmp(pcEPass, crypt(pcWord, acSalt))) {
			return 1;
		}
	}
#if HAVE_SHADOW
	if ('x' == pwd->pw_passwd[0] && '\000' == pwd->pw_passwd[1]) {
		spwd = getspnam(pwd->pw_name);
		acSalt[0] = spwd->sp_pwdp[0];
		acSalt[1] = spwd->sp_pwdp[1];
		return 0 == strcmp(spwd->sp_pwdp, crypt(pcWord, acSalt));
	}
#endif
	acSalt[0] = pwd->pw_passwd[0];
	acSalt[1] = pwd->pw_passwd[1];
	return 0 == strcmp(pwd->pw_passwd, crypt(pcWord, acSalt));
}
#endif /* USE_MD5 */
