/* $Id: erename.c,v 3.3 1994/03/08 20:32:34 ksb Exp $
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>

#include "libtomb.h"


#if AIX_LIB_FAKE
extern int rename_r();
#else

#if USE_SYSSYSCALL
#include <sys/syscall.h>
#else
#include <syscall.h>
#endif

/* emulate rename(2) with syscall(2)					(ksb)
 */
int
rename_r(from, to)
char *from, *to;
{
	return syscall(SYS_rename, from, to);
}
#endif


/* trap a rename call and entomb the `to' side				(mjb)
 */
int
rename(pcFrom, pcTo)
char *pcFrom, *pcTo;
{
	auto struct stat statTo, statFrom;

	if (0 == STAT_CALL(pcTo, &statTo) && S_IFREG == (statTo.st_mode&S_IFMT) &&
	    0 == STAT_CALL(pcFrom, &statFrom) && statTo.st_dev == statFrom.st_dev &&
	    0 != statTo.st_size) {
		/* we can't rename here because the rename might not succeed
		 * (ksb)
		 */
		(void)_entomb(COPY, pcTo);
	}

	return rename_r(pcFrom, pcTo);
}
