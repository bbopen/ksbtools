/* $Id: machine.h,v 2.1 1997/11/17 01:35:10 ksb Exp $
 * meta source header leverage for Tee -- ksb
 */

#if !defined(HPUX) 
#if defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10)
#define HPUX	1
#else
#define HPUX	0
#endif
#endif

#if !defined(HAVE_RLIMIT)
#if (defined(SUN5)||defined(PTX4))
#define HAVE_RLIMIT	1
#else
#define HAVE_RLIMIT	0
#endif
#endif

#if !defined(NEED_GETDTABLESIZE)
#define NEED_GETDTABLESIZE	(defined(SUN5) || defined(HPUX9))
#endif

#if !defined(USE_SYS_INODE_H)
#define USE_SYS_INODE_H	HPUX
#endif
