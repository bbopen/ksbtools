/* $Id: machine.h,v 1.6 2006/01/27 21:08:55 ksb Exp $
 * lever for machine changes
 */

#if !defined(HAVE_STRERROR)
#define HAVE_STRERROR 1
#endif

#if !defined(USE_STRINGS)
#define USE_STRINGS	defined(SYSV)
#endif

#if !defined(HAVE_GETCWD)
#define HAVE_GETCWD	(defined(SYSV) || (defined(HPUX) && !defined(HPUX10)) || defined(IBMR2) || defined(SUN5) || defined(LINUX))
#endif

#if !defined(USE_STDLIB_H)
#define USE_STDLIB_H	(defined(IBMR2)||defined(PTX)||defined(FREEBSD)||defined(MSDOS)||defined(ULTRIX)||defined(BSDI)||defined(OPENBSD))
#endif

#if !defined(USE_MALLOC_H)
#define USE_MALLOC_H	(!(defined(HPUX9)||defined(HPUX10)))
#endif
