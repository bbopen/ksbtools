/* Machine dependent defintions
 * $Id: machine.h,v 2.47 2010/05/10 20:14:58 ksb Exp $
 */

/* if we have <strings.h> define this to 1, else define to 0
 */
#if !defined(USE_STRINGS)
#define USE_STRINGS	(defined(SUN4)||defined(DYNIX)||defined(EPIX)||defined(IRIX5))
#endif

#if !defined(NEED_UNISTD_H)
#define NEED_UNISTD_H	(defined(SUN5)||defined(PTX))
#endif

/* I don't think anyone sets sun_len anymore
 */
#if !defined(NEED_SUN_LEN)
#define NEED_SUN_LEN	0
#endif

/* do we have old style var args? otherwise, use stdarg (prefered)
 */
#if !defined(VARARGS)
#if SUN4||HPUX9||HPUX10
#define VARARGS		1
#else
#define VARARGS		0
#endif
#endif

#if USE_STRINGS
#define strchr	index
#define strrchr	rindex
#endif

#if !defined(USE_GETSPNAM)
#define USE_GETSPNAM	defined(SUN5)
#endif

/* Some older SUN5 hosts do, new ones do not -- give up on it
 */
#if USE_GETSPNAM
#if !defined(HAVE_GETSPUID)
#define HAVE_GETSPUID	0
#endif
#endif

/* Hackish Linux only kernel hook for NFS identity: we do not let you
 * configure it in the rules, we use the effective uid (gid).
 */
#if !defined(HAVE_SETFSUID)
#define HAVE_SETFSUID	defined(LINUX)
#endif
#if !defined(HAVE_SETFSGID)
#define HAVE_SETFGUID	(HAVE_SETFSUID)
#endif

/* Pmax's don't have LOG_AUTH
 */
#if !defined(LOG_AUTH)
#define LOG_AUTH LOG_WARNING
#endif

/* HPUX is missing 2 sysexits
 */
#if !defined(EX_CONFIG)
#define EX_CONFIG		78	/* Linux, BSD, Solaris have this */
#endif
#if !defined(EX_NOTFOUND)
#define EX_NOTFOUND		79	/* Solaris, and others have this */
#endif

#if !defined(HAVE_CRYPT_H)
#define HAVE_CRYPT_H	(defined(LINUX))
#endif

#if HAVE_CRYPT_H
#include <crypt.h>
#endif

#if !defined(USE_PAM)
#define USE_PAM		(defined(OPENBSD)||defined(NETBSD)||defined(FREEBSD)||defined(LINUX)||defined(SUN5)||defined(AIX)||defined(HPUX))
#endif

#if !defined(OP_PAM_IN)
#define OP_PAM_IN	"/etc/pam.d"
#endif
