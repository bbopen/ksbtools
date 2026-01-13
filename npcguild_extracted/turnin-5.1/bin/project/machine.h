/*
 * $Id: machine.h,v 4.18 1997/11/11 16:28:14 ksb Exp $
 *
 * These might be locally tuned						(ksb)
 */
#if defined(HPUX7) || defined(HPUX8) || defined(HPUX9) || defined(HPUX10)
#if !defined(HPUX)
#define HPUX	1
#endif
#endif

/*
 * send reports of requests for project additions:
 */
#if !defined(MAILTO)
#define MAILTO		"enroll-info"
#endif

/*
 * where the global database is kept for who can ctrl project(s)
 */
#if ! defined(TURNBASE)
#define TURNBASE	"/usr/local/lib/turnin.cf"
#endif

/*
 * where the list of active projects is kept in the submit directory
 */
#if ! defined(PROJLIST)
#define PROJLIST	"Projlist"
#endif

#if !defined(SECTIGNORE)
#define SECTIGNORE	"ALL"
#endif

#if ! defined(LOCKFILE)
#define LOCKFILE	".lock"
#endif

#if ! defined(SUBPREFIX)
#define SUBPREFIX	"SUB_"
#endif

#define BIGTAR		5000	/* size of a large project (to compress)*/

/*
 * These might not change much						(ksb)
 */
#define MAXCHARS	2000	/* maximum line length in database	*/
#define SEP		':'	/* field separator for data base	*/
#define MAXCOURSENAME	32	/* max width of a course name		*/
#define MAXLOGINNAME	32	/* max chars in a login name		*/
#define MAXSUBDIRNAME	48	/* max chars in a subdir name		*/
#define MAXGROUPNAME	32	/* max chars in a group name		*/
#define MAXDIVSEC	32	/* max width of a div/sec name		*/

/* not kept in an online database -- no one nees it ?
 */
#define MAXLONGNAME	132	/* long name for the superuser		*/
#define MAXYESNO	24	/* a two word explain, `none yet'	*/

#define MAXPROJNAME	32	/* maximum chars in a project name	*/
#define MAXPROJECTS	32	/* maximum active projects/course	*/
#define MAXCOURSE	64	/* maximum courses in db (total)	*/

#if !defined(MAXPATHLEN)
#define MAXPATHLEN 1024
#endif

#if !defined(HAVE_NDIR) && !defined(HAVE_DIRENT) && !defined(HAVE_DIRECT)
#if defined(ETA10) || defined(V386)
#define HAVE_NDIR        1
#endif

#if defined(SUN5) || defined(CONSENSYS)
#define HAVE_DIRENT      1
#endif

/* if we have BSDDIR which name is the type? (struct direct) or (struct dirent)
 */
#if !defined(DIRENT)
#if defined(IBMR2) || defined(SUN5) || defined(CONSENSYS)
#define DIRENT		dirent
#else
#define DIRENT		direct
#endif
#endif	/* need to scandir */
#endif

#if !defined(HAVE_SETRESUID)
#define HAVE_SETRESUID	(defined(HPUX))
#endif

#if !defined(HAVE_NICE)
#define HAVE_NICE	(defined(SUN5)||defined(HPUX)||defined(CONSENSYS)||defined(LINUX))
#endif

#if !defined(HAVE_NAME)
#define HAVE_NAME	defined(pdp11)
#endif

/* 1->strings.h, 0->string.h
 */
#if !defined(HAVE_STRINGS)
#define HAVE_STRINGS	defined(bsd)
#endif

/* 1->lockf, 0-> flock
 */
#if !defined(USE_LOCKF)
#define USE_LOCKF	(defined(HPUX)||defined(SUN5)||defined(CONSENSYS))
#endif

#if !defined(NEED_UNISTD_H)
#define NEED_UNISTD_H	defined(LINUX)
#endif

/* make up an strerror function?
 */
#if !defined(HAVE_STRERROR)
#if PTX||NEXT2||IBMR2||HPUX||SUN5||CONSENSYS||FREEBSD||LINUX
#define HAVE_STRERROR	1
#else
#define HAVE_STRERROR	0
#endif
#endif

#if !defined(WAIT_T)
#if defined(SUN3) || defined(SUN4) || defined(bsd)
#define WAIT_T		union wait
#define WECODE(Ms)	(Ms.w_retcode)
#else
#define WAIT_T		int
#define WECODE(Ms)	(((Ms)>>8) & 0xff)
#endif
#endif

#if !defined(HAVE_PUTENV)
#define HAVE_PUTENV	(defined(SUN4) || defined(SUN5) || defined(CONSENSYS))
#endif

#if defined(bsd)
#define strchr	index
#define strrchr	rindex
#endif

#if !defined(NCARGS)
#define NCARGS	ARG_MAX
#endif
