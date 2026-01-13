/* $Id: entomb.h,v 3.0 1992/04/22 13:23:16 ksb Stab $
 */
extern int Entomb();

extern char *progname;

/*
 * the number you get from major(statbuf.st_dev)
 */
#if defined SUNOS && SUNOS < 40
#define NFS_MAJ_DEV	(255)
#endif	/*SUNOS*/
#if defined SUNOS && SUNOS >= 40
#define NFS_MAJ_DEV	(130)
#endif	/*SUNOS*/
#if defined DYNIX
#define NFS_MAJ_DEV	(255)
#endif
#if defined ULTRIX
#define NFS_MAJ_DEV	(78)
#endif

/* the default */
#if ! defined NFS_MAJ_DEV
#define NFS_MAJ_DEV	(255)
#endif
