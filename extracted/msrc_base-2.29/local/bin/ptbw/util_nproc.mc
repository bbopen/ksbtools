/* $Id: util_nproc.mc,v 1.2 2006/09/05 14:26:15 ksb Exp $
 */
%@if "FREEBSD" in HostType
#include <sys/sysctl.h>
%@elif "HPUX11" in HostType || "HPUX10" in HostType
#include <sys/mpctl.h>
%@endif

unsigned long
%%K<util_nproc>1ev()
{
%@if "FREEBSD" in HostType
	/* FreeBSD uses sysctl()
	 */
	auto int mib[4];
	auto size_t wLen, wBuffer;
	auto unsigned long lRet;

	lRet = 0;
	wBuffer = sizeof(lRet);
	wLen = sizeof(mib)/sizeof(mib[0]);
#if defined(HOSTOS) && HOSTOS < 40200
	sysctlbyname("hw.cpu", &lRet, &wBuffer, (void *)0, 0);
#else
	sysctlnametomib("hw.ncpu", mib, &wLen);
	sysctl(mib, wLen, &lRet, &wBuffer, (void *)0, 0);
#endif
	return lRet;
%@elif "AIX" in HostType || "IBMR2" in HostType
	/* AIX/IBMR2 uses sysconf with a different macro
	 */
	return (unsigned long)sysconf(_SC_NPROCESSORS_CONF);
%@elif "HPUX11" in HostType || "HPUX10" in HostType
	/* HPUX11/HPUX10 use strange multiprocessor control calls
	 */
	return (unsigned long)mpctl(MPC_GETNUMSPUS, 0, getpid());
%@elif "LINUX" in HostType
	/* AS/LINUX uses Posix, as far as I can tell
	 */
	return (unsigned long)sysconf(_SC_NPROCESSORS_ONLN);
%@elif "SUN5" in HostType
	/* Else try Posix by default
	 */
	return (unsigned long)sysconf(_SC_NPROCESSORS_ONLN);
%@endif
}
