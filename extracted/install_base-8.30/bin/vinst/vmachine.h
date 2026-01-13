/* $Id: vmachine.h,v 1.2 2008/11/13 01:28:17 ksb Exp $
 */

#if !defined(HAVE_MKSTEMP)
#define HAVE_MKSTEMP	(defined(FREEBSD)||defined(OPENBSD)||defined(NETBSD)||defined(LINUX)||defined(SUN5)||defined(HPUX11))
#endif
