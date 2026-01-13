/*
 * $Id: install.h,v 8.2 1997/02/16 20:03:49 ksb Exp $
 * Copyright 1990 Purdue Research Foundation, West Lafayette, Indiana
 * 47907.  All rights reserved.
 *
 * Written by Kevin S Braunsdorf, ksb@cc.purdue.edu, purdue!ksb
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. Neither the authors nor Purdue University are responsible for any
 *    consequences of the use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Credit to the authors and Purdue
 *    University must appear in documentation and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 */

/*
 * all the externs and positioning for install				(ksb)
 */
#define PATCH_LEVEL	1


#if STRINGS
#include <strings.h>

#define strchr	index
#define strrchr	rindex

#else
#include <string.h>

#endif


#if defined(SYSV) || defined(HPUX7) || defined(IBMR2) || defined(IRIX)

#if !defined(MAXPATHLEN)
#define MAXPATHLEN	256
#endif	/* already defined by someone		*/

#define vfork	fork		/* no vfork				*/
extern struct group *getgrent(), *getgrgid(), *getgrnam();
extern struct passwd *getpwent(), *getpwuid(), *getpwnam();

#endif	/* system 5				*/


#if defined(DYNIX)
#include <sys/universe.h>
#endif	/* universe stuff for symlinks		*/

extern int errno;		/* for perror(3)			*/
#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me)	(sys_errlist[Me])
#endif

#if !HAVE_ACCTTYPES
#define	uid_t int
#define gid_t int
#endif	/* find a uid/gid type			*/

#if defined(HAVE_SLINKS) ? HAVE_SLINKS : defined(S_IFLNK)

#define STAT_CALL	lstat	/* if symbolic links available		*/
#if !defined(HAVE_SLINKS)
#define HAVE_SLINKS	1
#endif

#if !defined(SLINKOK)
#define SLINKOK		FALSE	/* allow src to be a symlink?		*/
#endif	/* default source links to not allow	*/

#if !defined(DLINKOK)
#define DLINKOK		FALSE	/* allow dst to be a symlink?		*/
#endif	/* default destination links to not allow */

#else	/* we don't need to worry about lstat	*/

#define STAT_CALL	stat	/* symbolic links not available		*/
#if !defined(HAVE_SLINKS)
#define HAVE_SLINKS	0
#endif
#endif	/* if we have symlink preserving stat	*/


/* a crutch or two
 */
#define MAXTRIES	16		/* retrys on mktemp clashes	*/
#define	PATMKTEMP	"XXXXXX"	/* tack this on for mktemp(3)	*/
#undef TRUE
#undef FALSE
#define TRUE		(1)		/* you know, boolean like	*/
#define FALSE		(0)		/* you know, boolean like	*/
#define SUCCEED		(1)		/* our internal success code	*/
#define FAIL		(-1)		/* our internal failure code	*/
#define EXIT_FSYS	0x40		/* failed system call		*/
#define EXIT_OPT	0x41		/* bad option or data		*/
#define EXIT_INTER	0x42		/* internal error		*/

#define PERM_RWX(x)	((x) & 0777)
#define PERM_BITS(x)	((x) & 07777)
#define SAFEMASK	(~(S_ISUID|S_ISGID|S_ISVTX))


/* defaults that the installer gets if here doesn't -D define them
 * or set them in configure.h, or set them on the command line...
 */

#if INHERIT_OK

/* in this case we will copy modes whenever we get a chance,
 * (we use a (char*)0 to record `inherit').
 */

/* what to do for plain files we are installing
 */
#if !defined(DEFOWNER)
#define DEFOWNER	(char *)0
#endif

#if !defined(DEFGROUP)
#define DEFGROUP	(char *)0
#endif

#if !defined(DEFMODE)
#define DEFMODE		(char *)0
#endif

/* for the directory option (-d)
 */
#if !defined(DEFDIROWNER)
#define DEFDIROWNER	(char *)0
#endif

#if !defined(DEFDIRGROUP)
#define DEFDIRGROUP	(char *)0
#endif

#if !defined(DEFDIRMODE)
#define DEFDIRMODE	(char *)0
#endif

/* for the backup directory we might have to construct along the way
 */
#if !defined(ODIROWNER)
/* purge doesn't want such an unsafe general case -- so we
 * set _should_ the OLD dir owner to root by default ("bin" if you like)
 * we don't do this because I pretty sure only purge would benefit, so
 * I'll fix it in purge. -- ksb
 */
#define ODIROWNER	(char *)0
#endif

#if !defined(ODIRGROUP)
#define ODIRGROUP	(char *)0
#endif

#if !defined(ODIRMODE)
#define ODIRMODE	(char *)0
#endif

#else	/* try to guess based on what ksb thinks is best */

/* otherwise we want to set all these default modes here...
 * first for installed files (for the super user)
 */
#if !defined(DEFOWNER)
#define DEFOWNER	"root"
#endif

#if !defined(DEFGROUP)
#define DEFGROUP	"bin"
#endif

#if !defined(DEFMODE)
#define DEFMODE		"0755"
#endif

/* installed directory defauilt owner, group and mode
 */
#if !defined(DEFDIROWNER)
#define DEFDIROWNER	DEFOWNER
#endif

#if !defined(DEFDIRGROUP)
#define DEFDIRGROUP	DEFGROUP
#endif

#if !defined(DEFDIRMODE)
#define DEFDIRMODE	"0755"
#endif

/* default backup dir owner, group, and mode
 */
#if !defined(ODIROWNER)
#define ODIROWNER	DEFDIROWNER
#endif

#if !defined(ODIRGROUP)
#define ODIRGROUP	(char *)0
#endif

#if !defined(ODIRMODE)
#define ODIRMODE	(char *)0
#endif

#endif	/* must never inherit modes		*/


/* default backup directory name, I think everyone looks at this define
 * no rather than a hard coded "OLD" (but I'm not 100% sure).
 */
#if !defined(OLDDIR)
#define OLDDIR		"OLD"
#endif

#if !defined(TMPINST)
#define TMPINST		"#instXXXXXX"
#endif	/* mktemp to get source on same device	*/

#if !defined(TMPBOGUS)
#define TMPBOGUS	"#bogusXXXXXX"
#endif	/* last link to a running binary wedge	*/

#if defined(INST_FACILITY)
#include <syslog.h>
#if !defined(MAXLOGLINE)
#define MAXLOGLINE	256
#endif	/* sprintf buffer size for syslog	*/
#endif	/* syslog changes to the system		*/

#if !defined(BLKSIZ)
#define BLKSIZ		8192	/* default blocksize for DoCopy()	*/
#endif	/* maybe not big enough			*/
