/* $Id: machine.h,v 1.3 2000/01/31 00:03:52 ksb Exp $
 */

/* standard 7-point style leverage for stat{,v}fs call we need
 * to get fs tunes.  This is way harder than it should be... -- ksb
 */

/* pick one, or more
 */
#if !defined(HAVE_STATVFS)
#define HAVE_STATVFS	defined(SUN5)
#endif

#if !defined(HAVE_STATFS)
#define HAVE_STATFS	(defined(FREEBSD)||defined(HPUX9)||defined(HPUX10)||defined(HPUX11))
#endif

#if !defined(HAVE_ANSI_EXTERN)
#define HAVE_ANSI_EXTERN	!(defined(HPUX11))
#endif

/* act on the choice to get system headers
 */
#if HAVE_STATFS

#if defined(HPUX9)||defined(HPUX10)||defined(HPUX11)
#include <sys/vfs.h>
#endif

#if defined(FREEBSD)
#include <sys/param.h>
#include <sys/mount.h>
#endif
#endif

#if HAVE_STATVFS

#if defined(SUN5)
#include <sys/types.h>
#include <sys/statvfs.h>
#endif
#endif

/* make them look the same, as much as we can
 */
#if !defined(STATFS_BUF)
#if HAVE_STATFS
#define STATFS_BUF	struct statfs
#endif
#endif
#if !defined(STATFS_BUF)
#if HAVE_STATVFS
#define STATFS_BUF	struct statvfs
#endif
#endif

#if !defined(STATFS_CALL)
#if HAVE_STATFS
#define STATFS_CALL	statfs
#endif
#endif
#if !defined(STATFS_CALL)
#if HAVE_STATVFS
#define STATFS_CALL	statvfs
#endif
#endif
