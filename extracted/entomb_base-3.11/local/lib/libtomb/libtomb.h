/* $Id: libtomb.h,v 3.43 2008/11/17 18:52:46 ksb Exp $
 * This header file translated where we are (S81, SUN{3,4}, VAX780...}
 * into how/what to do/use.
 */

#if !defined(HPUX) && (defined(HPUX7) || defined(HPUX8) || defined(HPUX9) || defined(HPUX10) || defined(HPUX11))
#define HPUX	1
#endif

/* who do we live as (need both number and name)
 */
#if !defined(TOMB_OWNER)
#define TOMB_OWNER	"charon"		/* getpwnam(this)	*/
#endif

/* NB: 807.807 was the login ksb used for a long time
 */
#if !defined(USE_OLD_UID)
#define USE_OLD_UID	0
#endif

#if !defined(TOMB_UID)
#if USE_OLD_UID
#define TOMB_UID	807			/* gets this uid	*/
#else
#define TOMB_UID	11520
#endif
#endif

#if !defined(USE_OLD_GID)
#define USE_OLD_GID	0
#endif

#if !defined(TOMB_GID)
#if USE_OLD_GID
#define TOMB_GID	807
#else
#define TOMB_GID	602
#endif
#endif

/* where will be be installed
 */
#if !defined(ENTOMB)
#define ENTOMB		"/usr/local/lib/entomb"
#endif

/* what directory under each mount-point is the tomb directory
 */
#if !defined(TOMB_NAME)
#define TOMB_NAME	"Tomb"
#endif

/* if ENTOMB environment variable is not set, use this
 */
#if !defined(DEFAULT_EV)
#define DEFAULT_EV	"no:*.o:a.out:core:y.output:y.tab.?:lex.yy.?"
#endif

/* `show me a pager and I'll show you a file' (less is better)
 */
#if !defined(DEFPAGER)
#define DEFPAGER "/usr/ucb/more"
#endif

/* do not turn this off
 */
#if !defined(GLOB_ENTOMB)
#define GLOB_ENTOMB	1
#endif



/* internal -- how should we entomb this file (mv or cp)
 */
#define RENAME		1
#define COPY		2
#define UNLINK		3
#define COPY_UNLINK	4

/* Error return values from entomb program
 * These can be |'d together, if you call entomb with more than on
 * file.  Not very useful if you get a `no tomb' and the files are
 * on more than on fs :-{.
 */
#define WE_CROAK	(1)	/* system error (no more space?)	*/
#define NOT_ENTOMBED	(2)	/* file was not eligable (links?)	*/
#define NO_TOMB		(4)	/* no tomb on this device, stop asking	*/
#define NO_PATH		(8)	/* we couldn't find `entomb' to execve	*/

/* how to get mounted file systems
 */
#if ! (HAS_MTAB || HAS_GETMNT || HAS_GETMNTENT || HAS_VMNT || HAS_GETMNTINFO)
#if VAX780
#define HAS_MTAB	1
#endif
#if VAX8800
#define HAS_GETMNT	1
#endif
#if SUN3 || SUN4 || SUN5 || S81 || NEXT2 || HPUX8 || HPUX9 || HPUX10 || HPUX11 || LINUX
#define HAS_GETMNTENT	1
#endif
#if IBMR2
#define HAS_VMNT	1
#endif
#if FREEBSD || NETBSD || OPENBSD || BSDI
#define HAS_GETMNTINFO	1
#endif
#endif /* need a way to read mounted file systems */

/* this moves the name open/creat to 'junk' while we include fcntl.h
 */
#if !defined(MOVE_SYS_FUNCS)
#define MOVE_SYS_FUNCS	(defined(IBMR2)||defined(FREEBSD))
#endif

#if !defined(HAVE_UNIONWAIT)
#define HAVE_UNIONWAIT	(defined(VAX780)||defined(VAX8800)||defined(SUN3)||defined(SUN4)||defined(S81)||defined(NEXT2)||defined(NETBSD)||defined(BSDI))
#endif

/* machines that have union wait usually have wait4/waitpid
 * (If an OS has wait4 but not waitpid we should flame them. --ksb)
 */
#if !defined(HAVE_WAITPID)
#define HAVE_WAITPID	(defined(SUN5)||defined(HPUX9)||defined(HPUX10)||defined(HPUX11)||HAVE_UNIONWAIT)
#endif

#if !defined(HAVE_NDIR)
#define HAVE_NDIR        (defined(ETA10)||defined(V386))
#endif

#if !defined(HAVE_DIRENT)
#define HAVE_DIRENT      (defined(EPIX)||defined(SUN5)||defined(LINUX)||defined(FREEBSD))
#endif

#if !defined(DIRENT)
#if defined(IBMR2) || HAVE_DIRENT
#define DIRENT		dirent
#else
#define DIRENT		direct
#endif
#endif	/* need to scandir */

#if !defined(NEED_PROTO_DEFINE)
#define NEED_PROTO_DEFINE (defined(HPUX9)||defined(LINUX)||defined(NETBSD)||defined(OPENBSD)||defined(SUN5)||defined(HPUX10)||defined(HPUX11))
#endif

#if !defined(USE_STATFS)
#define USE_STATFS	(defined(IBMR2)||defined(NEXT2)||defined(FREEBSD)||defined(NETBSD)||defined(OPENBSD)||defined(LINUX)||defined(BSDI))
#endif	/* need to get file system type */

#if !defined(USE_STRINGS)
#define USE_STRINGS	(defined(HPUX9))
#endif

#if !defined(USE_SYSSYSCALL)
#define USE_SYSSYSCALL	(defined(SUN5)||defined(FREEBSD)||defined(NETBSD)||defined(OPENBSD)||defined(BSDI))
#endif

#if !defined(USE_SYSEXIT_H)
#define USE_SYSEXIT_H	(!defined(SUN5))
#endif

#if !defined(USE_FCNTL_H)
#define USE_FCNTL_H	(defined(SUN5)||defined(FREEBSD))
#endif

#if !defined(AVOID_OPEN_PROTO)
#define AVOID_OPEN_PROTO	(defined(NETBSD)||defined(FREEBSD)||defined(BSDI))
#endif

#if HAS_GETMNTENT
#if !defined(USE_SYS_MNTTAB_H)
#define USE_SYS_MNTTAB_H	(defined(SUN5))
#endif
#endif

#if !defined(HAVE_STRERROR)
#define HAVE_STRERROR	(defined(NEXT2)||defined(SUN5)||defined(IRIX)||defined(FREEBSD)||defined(BSDI)||defined(LINUX)||defined(NETBSD))
#endif

#if defined(SYSV) || defined(HPUX) || defined(IBMR2) || defined(SUN5)
#define getwd(Mp) getcwd(Mp,MAXPATHLEN)	/* return pwd		*/
extern char *getcwd();
#else
extern char *getwd();
#endif

/* if we need this root may not be able to restore files with unrm
 */
#if !defined(USE_SAVED_UID)
#define USE_SAVED_UID	(defined(HPUX8) || defined(HPUX9) || defined(HPUX10) || defined(HPUX11))
#endif

/* we should include <stat.h> before this file
 */
#if defined(S_IFLNK)
#define STAT_CALL	lstat
#else
#define STAT_CALL	stat
#endif

/* Preend trys to change argv to show actions.
 * What should we full unused chars in argv[0] with?
 */
#if !defined(PS_FILL)
#if defined(IBMR2)||defined(VAX780)
#define PS_FILL	' '
#else
#define PS_FILL	'\000'
#endif
#endif

#if !defined(HAVE_STDLIB_H)
#define HAVE_STDLIB_H	(defined(SUN5)||defined(IBMR2)||defined(LINUX)||defined(BSDI))
#endif

#if defined(LINUX) && !defined(POSIX)
#define POSIX
#endif

/* Posix uses mode_t for modes.
 */
#if defined(VAX8800)||defined(VAX780)||defined(S81)
typedef int mode_t;
#endif

extern char libtomb_version[];
