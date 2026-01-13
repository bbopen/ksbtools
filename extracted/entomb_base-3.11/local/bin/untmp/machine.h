/* $Id: machine.h,v 1.3 2009/01/28 14:20:44 ksb Exp $
 */

#if !defined(HPUX) && (defined(HPUX7)||defined(HPUX8)||defined(HPUX9))
#define HPUX	1
#endif

/* do we have scandir,readdir and friends in <sys/dir.h>, else look for
 * <ndir.h>
 */
#if !defined(HAVE_NDIR)
#define HAVE_NDIR	(defined(ETA10)||defined(V386))
#endif

#if !defined(HAVE_DIRENT)
#define HAVE_DIRENT	(defined(LINUX)||defined(EPIX)||defined(SUN5)||defined(FREEBSD)||defined(PARAGON))
#endif

/* Some versions of Solaris don't have d_namlen
 */
#if !defined(HAVE_DIRENT_NAMELEN)
#if defined(HAVE_DIRENT)
#define HAVE_DIRENT_NAMELEN	(defined(FREEBSD)||defined(LINUX)||defined(EPIX)||defined(PARAGON))
#else
#define HAVE_DIRENT_NAMELEN	0
#endif
#endif

#if !defined(DIRENT)
#if HAVE_DIRENT || defined(IBMR2)
#define DIRENT		dirent
#else
#define DIRENT		direct
#endif
#endif

/* we used to use pD->d_namlen -- under EPIX -systype bsd we still can!
 */
#if !defined(DIRENTSIZE)
#define DIRENTSIZE(Mi)	(sizeof(struct DIRENT) - 1)
#endif

#if !defined(D_NAMELEN)
#if defined(_D_EXACT_NAMLEN)
#define D_NAMELEN(MpD)	_D_EXACT_NAMLEN(MpP)
#else
#if defined(HAVE_DIRENT_NAMELEN)
#define D_NAMELEN(MpD)	((MpD)->d_namlen)
#else
#define D_NAMELEN(MpD)	((MpD)->d_reclen - DIRENTSIZE(0))
#endif /* glibc tells me or have dirent */
#endif /* glibc knows already */
#endif /* the system told me */


#if !defined(HAVE_STRERROR)
#define HAVE_STRERROR	(defined(NEXT2)||defined(IBMR2)||defined(HPUX7)||defined(HPUX8)||defined(SUN5)||defined(NETBSD)||defined(IRIX5)||defined(FREEBSD)||defined(LINUX))
#endif

#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif
