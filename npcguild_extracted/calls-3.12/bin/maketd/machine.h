/* $Id: machine.h,v 4.15 1999/01/10 01:31:49 ksb Exp $
 * The macros below are used to port maketd to variaous hosts
 * fill in the ones you know. {In the Makefile?}
 */
#if !defined(HPUX)
#if defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10)
#define HPUX	1
#endif
#endif

#if !defined(IRIX) && (defined(IRIX4) || defined(IRIX5) || defined(IRIX6))
#define IRIX	1
#endif

#if !defined(ULTRIX) && (defined(ULTRIX4) || defined(ULTRIX5))
#define ULTRIX	1
#endif


/* how to run the C preprocessor
 */
#if !defined(PATH_CPP)
#if USE_CPP_M
#define PATH_CPP	"${CC-${cc-cc}} -M %I %F"
#else
#define PATH_CPP	"${CC-${cc-cc}} -E %I %F |sed -n 's@^# *[0-9]* *\"\\\\(.*\\\\)\"[ 0-9]*@%f: \\\\1@p'"
#endif
#endif

/* location of m4 macro preprocessor
 */
#if !defined(PATH_M4)
#define PATH_M4		"/usr/bin/m4 -M %I %F"
#endif

/* delete "Makefile.bak" after each run of maketd LLL cmdline option?
 */
#if !defined(DEL_BACKUP)
#define DEL_BACKUP	0
#endif

/* here we deduce some things about the system from the info above
 * (some of these may need special cases for your strange systems)
 */

#if !defined(USE_STRINGS)
#define USE_STRINGS	(defined(bsd))
#endif

#if HAVE_STRDUP
#define STRSAVE		strdup
#else
#define	STRSAVE(Mpch)	strcpy(malloc((unsigned)strlen(Mpch)+1),Mpch)
#endif

#if !defined(HAVE_STRERROR)
#define HAVE_STRERROR	(defined(NEXT2)||defined(IBMR2)||defined(HPUX)||defined(SUN5)||defined(NETBSD)||defined(IRIX5)||defined(IRIX6)||defined(PARAGON)||defined(FREEBSD)||defined(LINUX))
#endif

#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif

#if !defined(SIGRET_T)
#if S81 || VAX8800
#define SIGRET_T	int
#else
#define SIGRET_T	void
#endif
#endif

#ifndef USE_STDLIB
#if IBMR2||PTX||FREEBSD||MSDOS||ULTRIX
#define USE_STDLIB	1
#else
#define USE_STDLIB	0
#endif
#endif

#if !defined(USE_MALLOC_H)
#define USE_MALLOC_H	(defined(PTX)||defined(NETBSD)||defined(IBMR2)||defined(SUN5)||defined(MSDOS)||defined(IRIX)||defined(ULTRIX))
#endif

#if !defined(NEED_MALLOC_EXTERN)
#define NEED_MALLOC_EXTERN	(!USE_MALLOC_H&&!defined(NEXT2)&&!defined(IRIX)&&!defined(BSDI))
#endif

#if USE_STDLIB || defined(__STDC__)
#include <stdlib.h>
#endif

#if USE_UNISTD_H
#include <unistd.h>
#endif

#if USE_MALLOC_H
#include <malloc.h>
#else
#if NEED_MALLOC_EXTERN
extern char *malloc(), *calloc(), *realloc();
#endif
#endif
