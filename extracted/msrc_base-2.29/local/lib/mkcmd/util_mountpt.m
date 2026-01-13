#!mkcmd
# $Id: util_mountpt.m,v 8.20 2001/04/20 02:12:38 ksb Exp $
# Provide a function to check a directory to see if it is a		(ksb)
# mount-point for a presently mounted filesystem

from "<stdio.h>"
from "<sys/stat.h>"
from "<sys/param.h>"

%ih
extern int IsMountPt(/* char *pcDir */);
%%

%c
/* return 1 if the given node is a mount-point				(ksb)
 * return -1 if we can't tell, and 0 for not a mount-point
 */
int
IsMountPt(pcDir)
char *pcDir;
{
	register char *pcName, *pcMem;
	register int iLen, i;
	auto struct stat stThis, stDotdot;

	if ((char *)0 == pcDir || '\000' == *pcDir) {
		return 0;
	}

	/* slash is a mount-point (spelled // even)
	 */
	for (pcName = pcDir; '/' == *pcName; ++pcName)
		/* nada */;
	if ('\000' == *pcName)
		return 1;

	/* doesn't exist, or some such, it's not a mount-point
	 */
	if (-1 == lstat(pcDir, & stThis))
		return -1;

	/* build ${pcDir}/.., or ${pcDir%/*}
	 */
	iLen = (strlen(pcDir)|3)+5;
	pcMem = malloc(iLen);

	/* out of memory then, ouch, we don't know
	 */
	if ((char *)0 == pcMem)
		return -1;

	/* try ${pcDir}/.. if we can
	 * else try ${pcDir%/*} or "."
	 */
	(void)strcpy(pcMem, pcDir);
	if (iLen < MAXPATHLEN-4 && S_IFDIR == (S_IFMT&stThis.st_mode)) {
		(void)strcat(pcMem, "/..");
	} else if ((char *)0 == (pcName = strrchr(pcMem, '/'))) {
		(void)strcpy(pcMem, ".");
	} else {
		*pcName = '\000';
	}
	i = stat(pcMem, & stDotdot);
	(void)free((void *)pcMem);
	if (-1 == i)
		return -1;
	return stDotdot.st_dev != stThis.st_dev;
}
