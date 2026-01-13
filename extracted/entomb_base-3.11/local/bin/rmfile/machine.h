/* $Id: machine.h,v 2.2 2008/09/13 20:23:26 ksb Exp $
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

/* do we have scandir,readdir and friends in <sys/dir.h>, else look for
 * <ndir.h>
 */
#if !defined(HAS_NDIR)
#define HAS_NDIR	(defined(ETA10)||defined(V386))
#endif

#if !defined(HAS_DIRENT)
#define HAS_DIRENT	(defined(EPIX)||defined(SUN5)||defined(FREEBSD)||defined(PARAGON) )
#endif

#if !defined(DIRENT)
#if defined(IBMR2) || HAS_DIRENT
#define DIRENT		dirent
#else
#define DIRENT		direct
#endif
#endif

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

