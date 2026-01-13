/* $Id: machine.h,v 2.5 2000/01/26 16:15:39 ksb Exp $
 */
#if !defined(HPUX) && (defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10))
#define HPUX	1
#endif

#if !defined(IRIX) && (defined(IRIX4) || defined(IRIX5) || defined(IRIX6))
#define IRIX	1
#endif

#if !defined(ULTRIX) && (defined(ULTRIX4) || defined(ULTRIX5))
#define ULTRIX	1
#endif

/* the name of the owners database in each target directory
 */
#if !defined(OWNERSFILE)
#define OWNERSFILE	".owners"
#endif

#if !defined(HAVE_STRERROR)
#define HAVE_STRERROR	(defined(NEXT2)||defined(IBMR2)||defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10)||defined(SUN5)||defined(NETBSD)||defined(IRIX5)||defined(FREEBSD)||defined(ULTRIX)||defined(LINUX))
#endif

#if !defined(HAVE_GETSPNAM)
#define HAVE_GETSPNAM	(defined(SUN5))
#endif
 
#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif
extern int errno;

#if !defined(HAVE_STRDUP)
#define HAVE_STRDUP	(defined(AMIGA)||defined(SUN4)||defined(SUN5)||defined(LINUX))
#endif

#if ! HAVE_STRDUP
extern char *strdup();
#endif

#if !defined(USE_MEMMOVE)
#define	USE_MEMMOVE	(defined(SUN5))
#endif

#if !defined(USE_UNISTD_H)
#define USE_UNISTD_H	(defined(SUN5)||POSIX)
#endif

#if !defined(HAS_GETGROUPS)
#define HAS_GETGROUPS	(!(defined(ETA10)||defined(EPIX)||defined(V386)))
#endif

/* if you use MD5 to encrypt passwords, like with FreeBSD
 * force on the command line if DES is installed on your FreeBSD
 */
#if !defined(USE_MD5)
#define USE_MD5		defined(FREEBSD)
#endif

#if !HAS_GETGROUPS
extern int getgroups();
#endif

#if ! defined(S_ISBLK)
# define S_ISBLK(m)      (((m)&S_IFMT) == S_IFBLK)
# define S_ISCHR(m)      (((m)&S_IFMT) == S_IFCHR)
# define S_ISDIR(m)      (((m)&S_IFMT) == S_IFDIR)
# define S_ISFIFO(m)     (((m)&S_IFMT) == S_IFIFO)
# define S_ISREG(m)      (((m)&S_IFMT) == S_IFREG)
# define S_ISLNK(m)      (((m)&S_IFMT) == S_IFLNK)
# define S_ISSOCK(m)     (((m)&S_IFMT) == S_IFSOCK)
#endif

/* find the getwd/getcwd call that works
 * Some sysV machines have getwd, some getcwd, some none
 * If you do not have one, get one.
 * (maybe popen a /bin/pwd and read it into the buffer?)
 */
#if !defined(HAVE_GETWD) && !defined(HAVE_GETCWD)
#if defined(SYSV) || (defined(HPUX) && !defined(HPUX10)) || defined(IBMR2)
#define HAVE_GETCWD	1
#else
#define HAVE_GETWD	1
#endif
#endif

#if HAVE_GETCWD
#define getwd(Mp) getcwd(Mp,MAXPATHLEN)	/* return pwd		*/
#endif

/* do we have scandir,readdir and friends in <sys/dir.h>, else look for
 * <ndir.h>
 */
#if !defined(HAS_NDIR)
#define HAS_NDIR        (defined(ETA10)||defined(V386))
#endif

#if !defined(HAS_DIRENT)
#define HAS_DIRENT      defined(SUN5)
#endif

/* if we have BSDDIR which name is the type? (struct direct) or (struct dirent)
 */
#if !defined(DIRENT)
#if defined(IBMR2) || defined(SUN5)
#define DIRENT		dirent
#else
#define DIRENT		direct
#endif
#endif	/* need to scandir */

/* we used to use pD->d_namlen -- under EPIX -systype bsd we still can!
 */
#if !defined(D_NAMELEN)
#if HAS_DIRENT
#if !defined(DIRENTSIZE)
#define DIRENTSIZE(Mi)	(sizeof(struct DIRENT) - 1)
#endif
#define D_NAMELEN(MpD)	((MpD)->d_reclen - DIRENTSIZE(0))
#else
#define D_NAMELEN(MpD)	((MpD)->d_namlen)
#endif
#endif

/* we need to check inode numbers
 */
#if !defined(D_INODE)
#if HAS_DIRENT
#define D_INODE(MpD)	((MpD)->d_ino)
#else
#define D_INODE(MpD)	((MpD)->d_fileno)
#endif
#endif

#if !defined(MAXPATHLEN)
#define MAXPATHLEN	1024
#endif

#if !defined(R_OK)
#define R_OK		4
#endif

#if !defined(NGROUPS)
#define NGROUPS		8
#endif

#if !defined(HAS_GETPW_EXTERNS)
#define HAS_GETPW_EXTERNS	(defined(IBMR2))
#endif

#if USE_UNISTD_H
#include <unistd.h>
#else
#if HAVE_GETWD
extern char *getwd();
#endif
#endif

#if !defined(HAVE_SCM_RIGHTS)
#if SUN5
#define HAVE_SCM_RIGHTS       0
#else
#define HAVE_SCM_RIGHTS       (defined(SCM_RIGHTS) || defined(LINUX))
#endif
#endif
