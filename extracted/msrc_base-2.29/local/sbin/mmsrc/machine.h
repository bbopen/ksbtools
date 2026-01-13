/* $Id: machine.h,v 1.11 2008/10/17 18:21:20 ksb Exp $
 * Every product needs one.						(ksb)
 */

#if !defined(HPUX)
#define HPUX	(defined(HPUX9)||defined(HPUX10)||defined(HPUX11))
#endif

/* Be compatible with the older shell version of msrc (smsrc), use the "dmz"
 * script to do that now days.
 */
#if !defined(SMSRC_COMPAT)
#define SMSRC_COMPAT	0
#endif

#if !defined(PATH_HXMD)
#define PATH_HXMD	"hxmd"
#endif

#if !defined(PATH_PTBW)
#define PATH_PTBW	"ptbw"
#endif

#if !defined(PATH_MAKE)
#define PATH_MAKE	"make"
#endif

#if !defined(MAP_SUFFIX)
#define MAP_SUFFIX	".host"
#endif

#if !defined(INCLUDE_SUFFIX)
#define INCLUDE_SUFFIX	".hxmd"
#endif

/* Old RedHat 6.2 hosts need the emulation for mkdtemp, but I don't
 * have a good way to compute the need here.  Autoconfig can do it
 * better.  Sadly the come with an autoconfig that can't, as well.
 * So we put it in the Makefile with a -DNEED_MKDTEMP in CDEFS. -- ksb
 */
#if !defined(NEED_MKDTEMP)
#if defined(HAVE_MKDTEMP)
#define NEED_MKDTEMP	(!(HAVE_MKDTEMP))
#else
#define NEED_MKDTEMP	(USE_MSRC_DEFAULT ? (defined(SUN5)||defined(IBMR2)||defined(HPUX11)) : 1)
#endif
#endif /* set need mkdtemp */

#if !defined(NEED_SETENV)
#if defined(HAVE_SETENV)
#define NEED_SETENV	(!(HAVE_SETENV))
#else
#define NEED_SETENV	(USE_MSRC_DEFAULT ? (defined(SUN5)||defined(HPUX)||defined(IBMR2)) : 1)
#endif
#endif

#if !defined(HAVE_ARC4RANDOM)
#define HAVE_ARC4RANDOM	(USE_MSRC_DEFAULT ? (defined(FREEBSD)||defined(NETBSD)||defined(OPENBSD)) : 0)
#endif

#if !defined(EMU_ARC4RANDOM)
#define EMU_ARC4RANDOM	(!(HAVE_ARC4RANDOM))
#endif

/* We need this, but I hate the kludge, should use sizeof not +2
 */
#if !defined(EMU_SOCKUN_LEN)
#define EMU_SOCKUN_LEN(Mpc)	(strlen(Mpc)+2)
#endif

#if NEED_MKDTEMP
extern char *mkdtemp(char *);
#endif

#if NEED_SETENV
#include "setenv.h"
#endif

#if !defined(USE_MALLOC_H)
#define USE_MALLOC_H	(defined(HAVE_MALLOC_H)||defined(IBMR2)||defined(AIX)||defined(LINUX)||defined(HPUX10)||defined(HPUX11))
#endif

/* HPUX is missing 2 sysexits
 */
#if !defined(EX_CONFIG)
#define EX_CONFIG		78	/* Linux, BSD, Solaris have this */
#endif
#if !defined(EX_NOTFOUND)
#define EX_NOTFOUND		79	/* Solaris, and others have this */
#endif
