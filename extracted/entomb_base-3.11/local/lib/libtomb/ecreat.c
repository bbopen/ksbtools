/* $Id: ecreat.c,v 3.5 1997/11/12 05:10:11 ksb Exp $
 */
#include <sys/types.h>
#include <sys/param.h>

#include "libtomb.h"

#if MOVE_SYS_FUNCS
#define creat	junk
#endif

#include <sys/file.h>

#if AIX_LIB_FAKE
extern int creat_r();
#else

#if USE_SYSSYSCALL
#include <sys/syscall.h>
#else
#include <syscall.h>
#endif

#if defined(SYS_creat)
/* emulate creat(2) with syscall					(ksb)
 */
int
creat_r(pcName, mode)
char *pcName;
mode_t mode;
{
	return syscall(SYS_creat, pcName, mode);
}
#endif
#endif

#if MOVE_SYS_FUNCS
#undef creat
#endif

#if defined(SYS_creat)
/* fake out creat(2)							(mjb)
 */
#if NEED_PROTO_DEFINE
int
creat(__const char *pcName, mode_t mode)
#else
int
creat(pcName, mode)
char *pcName;
mode_t mode;
#endif
{
	/* if the copy to the tomb fails we just ignore it
	 * I guess we could try something else, but what?  (chmod o+r?)
	 */
	(void)_entomb(COPY, pcName);

	return creat_r(pcName, mode);
}
#endif
