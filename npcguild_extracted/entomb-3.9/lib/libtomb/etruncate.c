/* $Id: etruncate.c,v 3.3 1994/03/08 20:32:36 ksb Exp $
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>

#include "libtomb.h"

#if defined(SYS_truncate)

#if AIX_LIB_FAKE
extern int truncate_r();
#else

#if USE_SYSSYSCALL
#include <sys/syscall.h>
#else
#include <syscall.h>
#endif

/* emulate truncate(2) with syscall(2)					(ksb)
 */
int
truncate_r(pcPath, iLength)
char *pcPath;
int iLength;
{
	return syscall(SYS_truncate, pcPath, iLength);
}
#endif


/* fake out truncate							(mjb)
 */
int
truncate(pcPath, iLength)
char *pcPath;
int iLength;
{

	/*
	 * What should probably be done here is to make a copy of the
	 * file in the tomb area and then call the real truncate.  For
	 * now, we are going to assume that the user of truncate is a 
	 * knowledgeable Joe and only back up the file if he is truncating
	 * to zero length.
	 */
	if (0 == iLength) {
		(void)_entomb(COPY, pcPath);
	}

	return truncate_r(pcPath, iLength);
}

#endif
