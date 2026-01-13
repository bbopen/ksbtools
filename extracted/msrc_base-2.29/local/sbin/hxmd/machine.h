/* $Id: machine.h,v 1.20 2009/12/28 18:51:08 ksb Exp $
 * every ksb tool needs one of these
 */

#if !defined(NEED_SELECT_H)
#define	NEED_SELECT_H	(defined(_POSIX)||defined(LINUX))
#endif

/* Extra OS specific header files
 */
#if (defined(HPUX11)||defined(HPUX10)||defined(HPUX9))
#define getpagesize()	sysconf(_SC_PAGE_SIZE)
#else
#if NEED_SELECT_H
#include <sys/select.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#endif
#endif

#if FREEBSD
#include <paths.h>
#endif

/* I can't believe on Linux you can't pass mkstemp, mkdtemp any number
 * of X's in the template other than 6, how lame is that.  --ksb
 */
#if !defined(BROKEN_MKTEMP)
#define BROKEN_MKTEMP	(defined(LINUX))
#endif

/* Force a path to m4, when the one in the search path is lame, or rename
 * m4 as "gm4" or some-such.
 */
#if !defined(M4_SYS_PATH)
#define M4_SYS_PATH	"m4"
#endif

/* Force a path to make, when the one in the search path is lame, or rename
 * make as "gmake" or some-such.
 */
#if !defined(MAKE_SYS_PATH)
#define MAKE_SYS_PATH	"make"
#endif

/* Sigh.  Some m4's don't popdef more than one define in a single call
 * Sun's m4 does, but a lot of people use GNU m4 there -- ksb.
 */
#if !defined(M4_POPDEF_MANY)
#define	M4_POPDEF_MANY	(defined(FREEBSD))
#endif


#if !defined(M4_DEBUG_MASK)
#if defined(LINUX)
#define M4_DEBUG_MASK	"taeqcxflpiV"
#else
#if defined(FREEBSD)
#define M4_DEBUG_MASK	"taeqcxflV"
#else
#if defined(SUN5)||defined(IBMR2)	/* we know they can't :-( */
#define M4_DEBUG_MASK	""
#else	/* unknown - pass none */
#define M4_DEBUG_MASK	""
#endif	/* lame m4's that don't take -d at all */
#endif	/* known FreeBSD */
#endif  /* known Linux */
#endif  /* mask incoming $HXMD_M4_DEBUG flags to filter our -d to m4's -d */

/* We need to open /dev/null for a bit bucket, or empty data source
 */
#if !defined(NULL_DEVICE)
#if defined(_PATH_DEVNULL)
#define	NULL_DEVICE	_PATH_DEVNULL
#else
#define	NULL_DEVICE	"/dev/null"
#endif
#endif

/* emulation code
 */
#if !defined(EMU_MKDTEMP)
#define EMU_MKDTEMP	(!(defined(FREEBSD)))
#endif

#if !defined(EMU_FGETLN)
#define EMU_FGETLN	(!(defined(FREEBSD)))
#endif

#if !defined(EMU_SNPRINTF)
#define EMU_SNPRINTF	(defined(HPUX9)||defined(HPUX10))
#endif

/* We need this, but I hate the kludge, should use sizeof not +2
 */
#if !defined(EMU_SOCKUN_LEN)
#define EMU_SOCKUN_LEN(Mpc)	(strlen(Mpc)+2)
#endif

/* some mmap options are *BSD only
 */
#if !defined(MAP_NOSYNC)
#define MAP_NOSYNC	0
#endif
#if !defined(MAP_NOCORE)
#define MAP_NOCORE	0
#endif
#if !defined(MAP_FAILED)
#define MAP_FAILED ((void *)-1)
#endif

#if EMU_MKDTEMP
extern char *mkdtemp(char *);
#endif
#if EMU_SNPRINTF
extern int snprintf();
#endif

/* some sysexits are missing these
 */
#if !defined(EX_CONFIG)
#define EX_CONFIG		78	/* Linux, BSD, Solaris have this */
#endif
#if !defined(EX_NOTFOUND)
#define EX_NOTFOUND		79	/* Solaris, and others have this */
#endif
