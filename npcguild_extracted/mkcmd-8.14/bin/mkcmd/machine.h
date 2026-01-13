/* $Id: machine.h,v 8.8 2000/02/20 16:52:21 ksb Exp $
 */
#if !defined(HPUX) && (defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10)||defined(HPUX11))
#define HPUX	1
#endif

#if !defined(IRIX) && (defined(IRIX4) || defined(IRIX5) || defined(IRIX6))
#define IRIX	1
#endif

#if !defined(ULTRIX) && (defined(ULTRIX4) || defined(ULTRIX5))
#define ULTRIX	1
#endif

#if !defined(DEFDIR)
#define DEFDIR	"/usr/local/lib/mkcmd:/usr/local/lib/mkcmd/type"
#endif

#if !defined(TILDEDIR)
#define TILDEDIR	!defined(MSDOS)
#endif

#if !defined(USE_STDLIB)
#if IBMR2||PTX||FREEBSD||MSDOS||ULTRIX||BSDI||HPUX10
#define USE_STDLIB	1
#else
#define USE_STDLIB	0
#endif
#endif

#if !defined(USE_STRINGS)
#define USE_STRINGS	0
#endif

#if !defined(USE_UNISTD_H)
#define USE_UNISTD_H	(defined(FREEBSD)||defined(HPUX9)||defined(HPUX10)||defined(HPUX11))
#endif

#if !defined(HAVE_STRDUP)
#define HAVE_STRDUP	(defined(SUN5)||defined(SUN4)||defined(SUN3)||defined(NETBSD)||defined(AMIGA)||defined(HPUX)||defined(IRIX5)||defined(IRIX6)||defined(IBMR2)||defined(FREEBSD)||defined(PARAGON)||defined(MSDOS)||defined(CONSENSYS)||defined(BSDI)||defined(LINUX))
#endif

#if !HAVE_STRDUP
extern char *strdup();	/* emulation in list.c			*/
#endif

#if !defined(USE_MEMCPY)
#if V386||HPUX7||HPUX8||EPIX||PTX||SUN5||MSDOS||CONSENSYS
#define USE_MEMCPY	1
#else
#define USE_MEMCPY	0
#endif
#endif

#if USE_MEMCPY
#define bcopy(Ms1,Ms2,Mn)	memcpy(Ms2,Ms1,Mn)
#endif

#if !defined(HAVE_STRERROR)
#if PTX||NEXT2||IBMR2||HPUX||SUN5||NETBSD||PARAGON||MSDOS||FREEBSD||IRIX||CONSENSYS||LINUX
#define HAVE_STRERROR	1
#else
#define HAVE_STRERROR	0
#endif
#endif

#if !defined(HAVE_STRCASECMP)
#if PTX||MSDOS
#define HAVE_STRCASECMP	0
#else
#define HAVE_STRCASECMP	1
#endif
#endif

#if !defined(QSORT_ARG)
#if PTX||IBMR2||NEXT2
#define QSORT_ARG	const void
#else
#define QSORT_ARG	char
#endif
#endif

#if !defined(HAVE_SIMPLE_ERRNO)
#define HAVE_SIMPLE_ERRNO	!(defined(MSDOS)||defined(XENIX))
#endif

#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif

#if !defined(USE_MALLOC_H)
#define USE_MALLOC_H	(defined(PTX)||defined(NETBSD)||defined(IBMR2)||defined(SUN5)||defined(MSDOS)||defined(IRIX)||defined(ULTRIX)||defined(CONSENSYS)||defined(HPUX10)||defined(HPUX11))
#endif

/* silly msdos C compilers don't do "defined" quite correctly
 * expressions like "!defined(MSDOS)" don't work
 */
#if !defined(USE_SYS_PARAM_H)
#if MSDOS
#define USE_SYS_PARAM_H	0
#else
#define USE_SYS_PARAM_H	1
#endif
#endif

#if !defined(NEED_MALLOC_EXTERN)
#define NEED_MALLOC_EXTERN	(!USE_MALLOC_H && !defined(NEXT2) && !defined(IRIX) && !defined(BSDI))
#endif

#if !defined(NEED_OFFSET_TYPE)
#define	NEED_OFFSET_TYPE	defined(MSDOS)
#endif

/* Ulrtix4 const parameters are not allowed !
 */
#if !defined(BROKEN_CONST)
#define BROKEN_CONST		defined(ULTRIX4)
#endif

#if BROKEN_CONST
#define	const	/* broken -- removed */
#endif

#if HAVE_SIMPLE_ERRNO
extern int errno;
#endif

#if NEED_OFFSET_TYPE
typedef long off_t;
#endif

#if USE_STDLIB || defined(__STDC__)
#include <stdlib.h>
#endif

#if USE_UNISTD_H
#include <unistd.h>
#endif
