/* $Id: eunlink.c,v 3.2 1994/03/08 20:32:38 ksb Exp $
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>

#include "libtomb.h"


#if AIX_LIB_FAKE
extern int unlink_r();
#else

#if USE_SYSSYSCALL
#include <sys/syscall.h>
#else
#include <syscall.h>
#endif

/* emulate unlink for calls to the Real Thing				(ksb)
 */
int
unlink_r(pcName)
char *pcName;
{
	return syscall(SYS_unlink, pcName);
}
#endif

/* fake out user unlink calls to entomb the file			(mjb)
 */
int
unlink(pcName)
char *pcName;
{
	/* if the entombing system move the file to the tomb we are done
	 */
	if (0 == _entomb(RENAME, pcName)) {
		return 0;
	}
	/* else unlink this name */
	return unlink_r(pcName);
}
