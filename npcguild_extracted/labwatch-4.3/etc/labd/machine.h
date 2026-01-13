/* $Id: machine.h,v 3.26 1999/05/23 18:44:36 ksb Exp $
 */
#if !defined(HPUX)
#if defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10)
#define HPUX	1
#endif
#endif

#if defined(LINUX) && !defined(POSIX)
#define POSIX 1
#endif

#if !defined(NEED_GNU_TYPES)
#define NEED_GNU_TYPES	defined(LINUX)
#endif

#if !defined(USE_LINUX_SYSINFO)
#define USE_LINUX_SYSINFO	defined(LINUX)
#endif

#if !defined(VMUNIX)
#if SUN5
#define VMUNIX	"/dev/ksyms"
#else
#if FREEBSD
#define VMUNIX	"/kernel"
#else
#if BSDI
#define VMUNIX	"/bsd"
#else
#if NETBSD
#define VMUNIX	"/netbsd"
#else
#if NEXT2
#define VMUNIX	"/mach"
#else
#if HPUX10
#define VMUNIX	"/stand/vmunix"
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
#endif
#endif
#endif

/* if USERFS_PREFIX is defined, only filesystems beginning with that
 * string will be listed, and the string will be chopped off.  So if
 * you mount {/usera, /userb, /userc, ...} you can use "/user" to get
 * a list like "a b c".  If you use the automounter, you may need to
 * use "/tmp_mnt/user" instead.  Consult your mtab.
 *
 * if you need more complex behavior, write a new mapping in fslist.c
 * and don't define USERFS_PREFIX.
 */
#if !defined(USERFS_PREFIX)
#if S81 || VAX8800 || NEXT2
#define USERFS_PREFIX	"/user"
#else
#define USERFS_PREFIX	"/tmp_mnt/user"
#endif
#endif

/* used to get the list of connected users and on some systems to find the
 * boottime
 */
#if !defined(UTMP_FILE)
#if defined(_PATH_UTMP)
#define UTMP_FILE	_PATH_UTMP
#else
#define UTMP_FILE	"/etc/utmp"
#endif
#endif

/* used on some systems to find the boot time
 */
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

/* does this system have the boot time in the kernel?
 */
#if !defined(HAS_KBOOTTIME)
#define HAS_KBOOTTIME		(defined(SUN4)||defined(SUN3)||defined(FREEBSD)||defined(NETBSD)||defined(NEXT2)||defined(HPUX)||defined(IRIX5)||defined(IRIX6)||defined(S81)||defined(VAX8800)||defined(BSDI))
#endif

/* what is the type of the boottime variable in the kernel?
 */
#if HAS_KBOOTTIME
#if !defined(TIME_T_BOOTTIME)
#define TIME_T_BOOTTIME		(defined(S81)||defined(VAX8800))
#endif
#endif

/* if not we get it from UTMP or WTMP
 */
#if ! HAS_KBOOTTIME
#if !defined(BOOTTIME_FILE)
#if IBMR2 || SUN5
#define BOOTTIME_FILE	UTMP_FILE
#else
#define BOOTTIME_FILE	WTMP_FILE
#endif
#endif
#endif

/* some systems give us a macro which tells us if a utmp entry is a user
 * or not.  on systems that don't, we provide an alternative
 */
#if !defined(nonuser)
#if NETBSD || FREEBSD || BSDI
#define	nonuser(Mut)	0
#else
#if SUN5 || IBMR2 || HPUX9 || HPUX10
#define nonuser(Mut)    ((Mut).ut_type != USER_PROCESS)
#else
#define	nonuser(Mut)	0
#endif
#endif
#endif

#ifndef HAVE_STRERROR
#define HAVE_STRERROR	(defined(NEXT2)||defined(IBMR2)||defined(HPUX)||defined(SUN5)||defined(FREEBSD)||defined(NETBSD)||defined(IRIX5)||defined(IRIX6)||defined(BSDI)||defined(LINUX))
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
#if !defined(HAS_MTAB)
#define HAS_MTAB	1
#endif
#endif

#if VAX8800
#if !defined(HAS_GETMNT)
#define HAS_GETMNT	1
#endif
#endif

#if SUN3 || SUN4 || SUN5 || S81 || NEXT2 || HPUX || IRIX5 || IRIX6 || LINUX
#if !defined(HAS_GETMNTENT)
#define HAS_GETMNTENT	1
#endif
#if !defined(MTAB_PATH)
#if HPUX7 || HPUX8 || HPUX9 || HPUX10
#define MTAB_PATH	"/etc/mnttab"
#else
#define MTAB_PATH	"/etc/mtab"
#endif
#endif
#endif

#if IBMR2
#if !defined(HAS_VMNT)
#define HAS_VMNT	1
#endif
#endif

#if FREEBSD || NETBSD || BSDI
#if !defined(HAS_GETMNTINFO)
#define HAS_GETMNTINFO	1
#endif
#endif
#endif /* need a way to read mounted file systems */

#if HAS_GETMNTENT
#if !defined(USE_SYS_MNTTAB_H)
#define USE_SYS_MNTTAB_H	defined(SUN5)
#endif
#endif

/* how the name of the console device shows up in utmp
 */
#if !defined(NAME_CONSOLE)
#if IBMR2
#define NAME_CONSOLE		"t0"
#else
#define NAME_CONSOLE		"console"
#endif
#endif

/* we count tty's at startup to guess at how much of utmp to mmap in
 */
#if !defined(HAS_GETTTYENT)
#define HAS_GETTTYENT		(!(defined(SUN5)||defined(HPUX)||defined(IRIX5)||defined(IRIX6)||defined(LINUX)))
#endif

#if !defined(HAVE_GETDTABLESIZE)
#define HAVE_GETDTABLESIZE	(defined(SUN4)||defined(SUN3)||defined(NEXT2)||defined(IRIX5)||defined(IRIX6)||defined(S81)||defined(VAX8800)||defined(IBMR2)||defined(FREEBSD)||defined(LINUX))
#endif

#if !defined(HAVE_SELECT_H)
#define HAVE_SELECT_H	(defined(IBMR2))
#endif

#if !defined(USE_STRINGS)
#define USE_STRINGS		(defined(HPUX8))
#endif

#if !defined(USE_SYSINFO)
#define USE_SYSINFO	defined(LINUX)
#endif

#if !defined(USE_ELF)
#define USE_ELF			(defined(SUN5)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10)||defined(IRIX5)||defined(IRIX6)||defined(LINUX))
#endif

/* do kernel symbols have an _ prefixed?
 */
#if !defined(NEED_UNDERBAR)
#define NEED_UNDERBAR		(!defined(IBMR2))
#endif

/* do we have avenrunable instead of avenrun?
 */
#if !defined(USE_AVERUNNABLE)
#define USE_AVERUNNABLE		(defined(NETBSD)||defined(FREEBSD)||defined(BSDI))
#endif

#if USE_AVERUNNABLE && !defined(NAME_AVENRUN)
#define NAME_AVENRUN	"_averunnable"
#endif


#if !defined(HAVE_H_ERRLIST)
#define HAVE_H_ERRLIST  (defined(SUN4)||defined(SUN3)||defined(FREEBSD)||defined(NETBSD)||defined(IRIX5)||defined(IRIX6))
#endif

#if !defined(HAVE_STRDUP)
#define HAVE_STRDUP	(defined(SUN5)||defined(SUN4)||defined(SUN3)||defined(FREEBSD)||defined(NETBSD)||defined(AMIGA)||defined(HPUX)||defined(IRIX5)||defined(IRIX6)||defined(IBMR2)||defined(LINUX))
#endif

#if !HAVE_STRDUP
#define strdup(Ms)	(strcpy(malloc((strlen(Ms)|3)+1), (Ms)))
extern char *malloc();
#endif

#if HAVE_H_ERRLIST
extern int h_errno;
extern char *h_errlist[];
#define hstrerror(Me)   (h_errlist[Me])
#else
#define hstrerror(Me)   "host lookup error"
#endif

#if !defined(INADDR_LOOPBACK)
#define	INADDR_LOOPBACK	((unsigned long)0x7F000001)
#endif

/* gettimeofday() requires a TZ for "backward `compatibility'"
 */
#if !defined(GETTIMEOFDAY_BROKEN)
#define GETTIMEOFDAY_BROKEN (defined(FREEBSD)||defined(NETBSD)||defined(NEXT2)||defined(HPUX)||defined(SUN5)||defined(LINUX)||defined(BSDI))
#endif

#if !defined(SEEK_SET)
#define SEEK_SET	0
#endif

/* mmap() must be able to see changes in the file not made by us
 * ZZZ HPUX10 is a guess!
 */
#if !defined(USE_MMAP)
#define USE_MMAP	(!(defined(NEXT2)||defined(HPUX7)||defined(HPUX8)||defined(HPUX10)||defined(VAX8800)||defined(S81)))
#endif

#if !defined(MACH_HEADERS)
#define MACH_HEADERS	defined(NEXT2)
#endif

/* NB: XMIT_SCALE *MUST* agree with the values for labwatch and labstat
 */
#if !defined(XMIT_SCALE)
#define XMIT_SCALE	4096
#endif

#if !defined(HAVE_KNLIST)
#define HAVE_KNLIST	defined(IBMR2)
#endif

#if !defined(NLIST_SENTINAL_VALUE)
#if IBMR2
#define	NLIST_SENTINAL_VALUE	{ NULL }
#else
#define	NLIST_SENTINAL_VALUE	{ "" }
#endif
#endif

#if !defined(AVENRUN_T)
#if defined(HPUX) || defined(VAX8800)
#define AVENRUN_T	double
#if !defined(FSCALE)
#define FSCALE		1
#endif

#else
#if FREEBSD
#define AVENRUN_T	fixpt_t
#else
#if IBMR2 || LINUX
#define AVENRUN_T	unsigned long
#else
#define AVENRUN_T	long
#endif
#endif

#if !defined(FSCALE)
#if IBMR2
#define FSCALE		65536.0
#else
#if LINUX
#define FSCALE		65536
#else
#if IRIX5 || IRIX6 || S81
#define FSCALE		1024
#else
#define FSCALE		4096
#endif
#endif
#endif /* figure an FSCALE */
#endif /* figure a type */

#endif
#endif

#if !defined(USE_GETKMEMDATA)
#define USE_GETKMEMDATA	(defined(IBMR2))
#endif

/* on some hosts the FSCALE in the header files is *wrong*
 */
#if !defined(ADJ_FSCALE)
#if NEXT2
#define	ADJ_FSCALE	4
#endif
#endif

/* clear up warnings
 */
#if !defined(SIGRET_T)
#if SUN5 || HPUX || IBMR2 || FREEBSD || IRIX5 || IRIX6 || LINUX || BSDI || NEXT2
#define SIGRET_T	void
#else
#define SIGRET_T	int
#endif
#endif

#if !defined(MAXUDPSIZE)
#define MAXUDPSIZE	1400
#endif

#if !defined(LABLOG_SERVICE)
#define LABLOG_SERVICE	"lablog"
#endif

#if !defined(HAVE_STICKY_SIGNALS)
#define HAVE_STICKY_SIGNALS	(!(defined(HPUX)||defined(SUN5)))
#endif
