/* $Id: machine.h,v 3.3 2008/07/10 20:44:10 ksb Exp $
 * lever for machine changes
 */

#if !defined(HAVE_STRERROR)
#define HAVE_STRERROR 1
#endif

#if !defined(HAVE_STDLIB)
#define HAVE_STDLIB	1
#endif

#if !defined(NEED_FSTAB_H)
#define NEED_FSTAB_H	0
#endif

/* HPUX is missing 2 sysexits
 */
#if !defined(EX_CONFIG)
#define EX_CONFIG		78	/* Linux, BSD, Solaris have this */
#endif
#if !defined(EX_NOTFOUND)
#define EX_NOTFOUND		79	/* Solaris, and others have this */
#endif
