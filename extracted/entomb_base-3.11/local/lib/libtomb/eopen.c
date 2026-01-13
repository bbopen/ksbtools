/* $Id: eopen.c,v 3.12 2003/12/16 19:18:28 ksb Exp $
 */
#include <sys/types.h>
#include <sys/param.h>

#include "libtomb.h"

#if MOVE_SYS_FUNCS
#define open	junk
#endif

#if AVOID_OPEN_PROTO
#define  KERNEL 1	/* doesn't prototype open(...) */
#endif

#include <sys/file.h>

#if USE_FCNTL_H
#include <fcntl.h>
#endif


#if AIX_LIB_FAKE
extern int open_r();
#else

#if USE_SYSSYSCALL
#include <sys/syscall.h>
#else
#include <syscall.h>
#endif

/* use syscall to emulate the real open routine				(ksb)
 */
int
open_r(file, flags, mode)
char *file;
int flags;
mode_t mode;
{
	return syscall(SYS_open, file, flags, mode);
}
#endif

#if MOVE_SYS_FUNCS
#undef open
#endif

/* fake out open,							(mjb)
 */
#if NEED_PROTO_DEFINE
#include <stdarg.h>

int
open(__const char *file, int flags, ...)
{
	va_list		arg;
	mode_t		mode;

	va_start(arg, flags);
	mode = va_arg(arg, mode_t);
	va_end(arg);

#else

int
open(file, flags, mode)
const char *file;
int flags;
mode_t mode;
{

#endif

	/* only entomb if we are zorching the whole file
	 */
	if (0 != (flags & (O_CREAT|O_TRUNC))) {
		/* once again we could try a chmod +r?
		 */
		(void)_entomb(COPY, file);
	}

	return open_r(file, flags, mode);
}
