/* $Id: machine.h,v 2.5 1998/02/12 20:20:16 ksb Exp $
 */
#if !defined(VMUNIX)
#if SUN5
#define VMUNIX	"/kernel/unix"
#else
#if NETBSD
#define VMUNIX	"/netbsd"
#else
#if NEXT2
#define VMUNIX	"/mach"
#else
#if HPUX7 || HPUX8 || HPUX9
#define VMUNIX	"/hp-ux"
#else
#if IRIX5 || IRIX6 || IBMR2
#define VMUNIX	"/unix"
#else
#if S81
#define VMUNIX	"/dynix"
#else
#define VMUNIX	"/vmunix"
#endif
#endif
#endif
#endif
#endif
#endif
#endif

#if !defined(USERFS_PREFIX)
#if S81 || VAX8800 || NEXT2
#define USERFS_PREFIX	"/user"
#else
#define USERFS_PREFIX	"/tmp_mnt/user"
#endif
#endif

#if !defined(UTMP_FILE)
#if defined(_PATH_UTMP)
#define UTMP_FILE	_PATH_UTMP
#else
#define UTMP_FILE	"/etc/utmp"
#endif
#endif

#if !defined(WTMP_FILE)
#if defined(_PATH_WTMP)
#define WTMP_FILE	_PATH_WTMP
#else
#if IBMR2
#define WTMP_FILE	"/usr/adm/wtmp"
#else
#define WTMP_FILE	"/etc/wtmp"
#endif
#endif
#endif

#if ! HAS_KBOOTTIME
#if !defined(BOOTTIME_FILE)
#if IBMR2
#define BOOTTIME_FILE	UTMP_FILE
#else
#define BOOTTIME_FILE	WTMP_FILE
#endif
#endif
#endif

#if !defined(nonuser)
#if NETBSD
#define	nonuser(Mut)	0
#else
#if SUN5 || IBMR2
#define nonuser(Mut)	((Mut).ut_type != USER_PROCESS)
#else
#define	nonuser(Mut)	0
#endif
#endif
#endif

#ifndef HAVE_STRERROR
#define HAVE_STRERROR	(defined(NEXT2)||defined(IBMR2)||defined(HPUX7)||defined(HPUX8)||defined(SUN5)||defined(NETBSD)||defined(IRIX5)||defined(IRIX6)||defined(FREEBSD)||defined(LINUX))
#endif

extern int errno;

#if !HAVE_STRERROR
extern char *sys_errlist[];
# define strerror(Me) (sys_errlist[Me])
#endif

/* how to get mounted file systems
 */
#if ! (HAS_MTAB || HAS_GETMNT || HAS_GETMNTENT || HAS_VMNT || HAS_GETMNTINFO)
#if VAX780
#define HAS_MTAB	1
#endif
#if VAX8800
#define HAS_GETMNT	1
#endif
#if SUN3 || SUN4 || SUN5 || S81 || NEXT2 || HPUX7 || HPUX8 || HPUX9 || IRIX5 || IRIX6
#define HAS_GETMNTENT	1
#if !defined(MTAB_PATH)
#if HPUX7 || HPUX8 || HPUX9
#define MTAB_PATH	"/etc/mnttab"
#else
#define MTAB_PATH	"/etc/mtab"
#endif
#endif
#endif
#if IBMR2
#define HAS_VMNT	1
#endif
#if FREEBSD || NETBSD
#define HAS_GETMNTINFO	1
#endif
#endif /* need a way to read mounted file systems */

#if HAS_GETMNTENT
#if !defined(USE_SYS_MNTTAB_H)
#define USE_SYS_MNTTAB_H	defined(SUN5)
#endif
#endif

#if !defined(HAS_GETTTYENT)
#define HAS_GETTTYENT		(!(defined(SUN5)||defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(IRIX5)||defined(IRIX6)))
#endif

#if !defined(HAVE_GETDTABLESIZE)
#define HAVE_GETDTABLESIZE	(defined(SUN4)||defined(SUN3)||defined(NEXT2)||defined(IRIX5)||defined(IRIX6)||defined(S81)||defined(VAX8800)||defined(IBMR2))
#endif

#if !defined(HAVE_SELECT_H)
#define HAVE_SELECT_H		(defined(IBMR2))
#endif

#if !defined(USE_STRINGS)
#define USE_STRINGS		(defined(HPUX8))
#endif

#if !defined(HAS_KBOOTTIME)
#define HAS_KBOOTTIME		(defined(SUN4)||defined(SUN3)||defined(NETBSD)||defined(NEXT2)||defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(IRIX5)||defined(IRIX6)||defined(S81)||defined(VAX8800))
#endif

#if HAS_KBOOTTIME
#if !defined(TIME_T_BOOTTIME)
#define TIME_T_BOOTTIME		(defined(S81)||defined(VAX8800))
#endif
#endif

#if !defined(USE_ELF)
#define USE_ELF			(defined(SUN5)||defined(HPUX8)||defined(HPUX9)||defined(IRIX5)||defined(IRIX6))
#endif

#if !defined(NEED_UNDERBAR)
#define NEED_UNDERBAR		(!defined(IBMR2))
#endif

#if !defined(USE_AVERUNNABLE)
#define USE_AVERUNNABLE		(defined(NETBSD))
#endif

#if !defined(HAVE_H_ERRLIST)
#define HAVE_H_ERRLIST  (defined(SUN4)||defined(SUN3)||defined(NETBSD)||defined(IRIX5)||defined(IRIX6))
#endif

#if !defined(HAVE_STRDUP)
#define HAVE_STRDUP	(defined(SUN5)||defined(SUN4)||defined(SUN3)||defined(NETBSD)||defined(AMIGA)||defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(IRIX5)||defined(IRIX6)||defined(IBMR2))
#endif

#if !HAVE_STRDUP
#define strdup(Ms)	(strcpy(malloc((strlen(Ms)|3)+1), (Ms)))
extern char *malloc();
#endif

#if HAVE_H_ERRLIST
extern int h_errno;
extern char *h_errlist[];
#define hstrerror(Me)	(h_errlist[Me])
#else
#define hstrerror(Me)	"host lookup error"
#endif

#if !defined(INADDR_LOOPBACK)
#define	INADDR_LOOPBACK	((unsigned long)0x7F000001)
#endif

/* gettimeofday() requires a TZ for "backward `compatibility'"
 */
#if !defined(GETTIMEOFDAY_BROKEN)
#define GETTIMEOFDAY_BROKEN (defined(NETBSD)||defined(NEXT2)||defined(HPUX7)||defined(HPUX8)||defined(HPUX9))
#endif

#if !defined(SEEK_SET)
#define SEEK_SET	0
#endif

/* mmap() must be able to see changes in the file not made by us
 */
#if !defined(USE_MMAP)
#define USE_MMAP	(!(defined(NEXT2)||defined(HPUX7)||defined(HPUX8)||defined(VAX8800)||defined(S81)))
#endif

#if !defined(MACH_HEADERS)
#define MACH_HEADERS	defined(NEXT2)
#endif

/* NB: XMIT_SCALE *MUST* agree with the values for labwatch and labstat
 */
#if !defined(XMIT_SCALE)
#define XMIT_SCALE	4096
#endif

#if !defined(AVENRUN_T)
#if defined(HPUX7) || defined(HPUX8) || defined(HPUX9) || defined(VAX8800)
#define AVENRUN_T	double
#if !defined(FSCALE)
#define FSCALE		1
#endif

#else
#define AVENRUN_T	long
#if !defined(FSCALE)
#if IRIX5 || IRIX6 || S81
#define FSCALE		1024
#else
#define FSCALE		4096
#endif
#endif

#endif
#endif

/* on some hosts the FSCALE in the header files is *wrong*
 */
#if !defined(ADJ_FSCALE)
#if NEXT2
#define	ADJ_FSCALE	4
#endif
#endif

#if !defined(SIGRET_T)
#if SUN5 || HPUX7 || HPUX8 || HPUX9
#define SIGRET_T	void
#else
#define SIGRET_T	int
#endif
#endif

#if !defined(MAXUDPSIZE)
#define MAXUDPSIZE	1400
#endif
