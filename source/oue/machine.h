/* $Id: machine.h,v 2.1 2008/08/09 23:54:50 ksb Exp $
 */

/* HPUX is missing 2 sysexits
 */
#if !defined(EX_CONFIG)
#define EX_CONFIG		78	/* Linux, BSD, Solaris have this */
#endif
#if !defined(EX_NOTFOUND)
#define EX_NOTFOUND		79	/* Solaris, and others have this */
#endif
