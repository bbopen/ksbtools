/* $Id: match.h,v 5.7 2003/09/26 16:34:24 ksb Exp $
 */

/* REGEX is 1 is you have a later AT&T RegEx like (GNU) regcomp & regexec
 */
#if !defined(REGEX)
#if (defined(HPUX9)||defined(HPUX10)||defined(HPUX11)||defined(FREEBSD)||defined(HURD1)||defined(NETBSD)||defined(OPENBSD)||defined(SUN5)||defined(IBMR2)||defined(LINUX))
#define REGEX		1
#else
#define REGEX		0
#endif
#endif

/* REGCOMP is 1 if this is a POSIX system
 */
#if !defined(REGCOMP)
#define REGCOMP		0
#endif

/* REGCMP is 1 if your system has the regcmp() function.
 * This is normally the case for System 5. or later Solaris
 */
#if !defined(REGCMP)
#if defined(_iPSC386) || defined(SUN5) || defined(IRIX5) || defined(IRIX6)
#define REGCMP		1
#else
#define REGCMP		0
#endif
#endif

/* fall back to the older one
 */
#if !defined(RECOMP)
#if (!defined(REGCMP) && !defined(REGCOMP) && !defined(REGCOMP2))
#define RECOMP		1
#else
#define RECOMP		0
#endif
#endif

/* Solaris requires a funny library to get R.E. support, sigh.
 */
#if REGCMP && SUN5
#define USE_LIBGEN	1
#endif

extern int ReMatch(/* char *pcText, char *pcPat */);
extern char *ReParen(/* char *pcText, int iCount, char **pcEnd*/);
