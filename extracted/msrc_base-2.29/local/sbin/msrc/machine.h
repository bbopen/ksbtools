/* $Id: machine.h,v 1.8 2008/07/09 19:56:14 ksb Exp $
 * Every product needs one.						(ksb)
 */

/* Be compatible with the shell version of msrc (smsrc)
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

#if !defined(NEED_MKDTEMP)
#define NEED_MKDTEMP	(defined(SUN5)||defined(IBMR2))
#endif

#if NEED_MKDTEMP
extern char *mkdtemp(char *);
#endif

/* HPUX is missing 2 sysexits
 */
#if !defined(EX_CONFIG)
#define EX_CONFIG		78	/* Linux, BSD, Solaris have this */
#endif
#if !defined(EX_NOTFOUND)
#define EX_NOTFOUND		79	/* Solaris, and others have this */
#endif
