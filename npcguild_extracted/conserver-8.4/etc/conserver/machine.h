/* $Id: machine.h,v 8.6 2000/07/25 23:14:45 ksb Exp $
 * the rest of the file SHOULD BE machine.h style leverage		(ksb)
 */
#if !defined(HPUX) && (defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10)||defined(HPUX11))
#define HPUX	1
#endif

#if !defined(IRIX) && (defined(IRIX4) || defined(IRIX5) || defined(IRIX6))
#define IRIX	1
#endif

#if !defined(ULTRIX) && (defined(ULTRIX4) || defined(ULTRIX5))
#define ULTRIX	1
#endif

#if !defined(POSIX)
#define POSIX	defined(LINUX)
#endif

#if !defined(NEED_GNU_TYPES)
#define NEED_GNU_TYPES	(defined(LINUX) && !defined(POSIX))
#endif

#if !defined(DO_ANNEX)
#define DO_ANNEX	1
#endif

#if !defined(DO_VIRTUAL)
#define DO_VIRTUAL	1
#endif

#if DO_VIRTUAL
/* if the virtual console option is on we need a source to ptys,
 * the PUCC ptyd daemon is the best source be know, else fall back
 * on some emulation code?? (XXX)
 */
#if !defined(HAVE_PTYD)
#if LOCAL_PTYD
#define HAVE_PTYD	1
#else
#define HAVE_PTYD	(defined(S81)||defined(VAX8800))
#endif
#endif

#if !defined(HAVE_MASTER_PTY)
#define HAVE_MASTER_PTY	(!defined(PTX4))
#endif

#if !defined(HAVE_GETPSEUDO)
#define HAVE_GETPSEUDO	(defined(PTX2))
#endif

#if !defined(HAVE_PTSNAME)
#define	HAVE_PTSNAME 	(defined(PTX4))
#endif

#if !defined(HAVE_LDTERM)
#define HAVE_LDTERM	(defined(SUN5))
#endif

#if !defined(HAVE_TTCOMPAT)
#define HAVE_TTCOMPAT	(defined(SUN5))
#endif

#if !defined(HAVE_STTY_LD)
#define HAVE_STTY_LD	(defined(IRIX5) || defined(IRIX6))
#endif

#endif /* virtual (process on a pseudo-tty) console support */

#if (defined(PTX2) || defined(PTX4))
#define PTX
#endif

/* some machine specific details
 */
#if !defined(HAVE_SELECT_H)
#define HAVE_SELECT_H	(defined(IBMR2)||defined(PTX)||defined(LINUX))
#endif

#if !defined(USE_POSIX_TTY)
#define USE_POSIX_TTY	(defined(IBMR2))
#endif

#if !defined(HAVE_UWAIT)
#define HAVE_UWAIT	!(defined(IBMR2)||defined(SUN5)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10)||defined(HPUX11)||defined(PTX)||defined(IRIX5)||defined(IRIX6)||defined(LINUX))
#endif

#if !defined(HAVE_WAIT3)
#define HAVE_WAIT3	!(defined(SUN5)||defined(PTX))
#endif

/* if the encrypted passwd is in a shadow file, define HAVE_SHADOW 	(gregf)
 */
#if !defined(HAVE_SHADOW)
#define HAVE_SHADOW	(defined(PTX)||defined(SUN5))
#endif

/* bogus -- sun4.1.3_U1 C2 version has a shadow file but no interface	(ksb)
 * routine in libc (under Solaris or PTX it is getspnam()).  So if we
 * need an emulation we define this.  Also see PATH_SHADOW below
 */
#if !defined(NEED_SHADOW)
#define NEED_SHADOW	(defined(SUN4))
#endif

/* if we have to open the shadow passwd file we need the path to it
 */
#if !defined(PATH_SHADOW)
#if defined(SUN4)
#define PATH_SHADOW	"/etc/security/passwd.adjunct"
#else
#define PATH_SHADOW	"/etc/shadow"
#endif
#endif


/* we'd like to line buffer our output, if we know how
 */
#if !defined(USE_SETLINEBUF)
#define USE_SETLINEBUF	(!(defined(SUN5)||defined(HPUX)||defined(PTX)))
#endif

/* we'd like to line buffer our output, if we know how; PTX uses setvbuf (gregf)
 */
#if !defined(USE_SETVBUF)
#define USE_SETVBUF	(defined(PTX))
#endif

/* hpux doesn't have getdtablesize() and they don't provide a macro
 * in non-KERNEL cpp mode	XXX
 */
#if defined(HPUX) && !defined(HPUX10)
#define getdtablesize()	64
#endif

#if !defined(HAVE_SETSID)
#define HAVE_SETSID	(defined(IBMR2)||defined(SUN5)||defined(HPUX)||defined(PTX)||defined(IRIX5)||defined(IRIX6)||defined(LINUX))
#endif

/* should we use flock to keep multiple conservers from hurting each other?
 * PTX has lockf... should probably port code to work with this (gregf)
 */
#if !defined(USE_FLOCK)
#define USE_FLOCK	(!(defined(IBMR2)||defined(SUN5)||defined(HPUX)||defined(PTX)))
#endif

/* should we try to pop streams modules off?
 */
#if !defined(USE_STREAMS)
#define USE_STREAMS	(defined(SUN4)||defined(SUN5)||defined(PTX)||defined(IRIX5)||defined(IRIX6))
#endif

/* if we do not have old style tty emulation use termios.h
 */
#if !defined(USE_TERMIO)
#define USE_TERMIO	(defined(ETA10)||defined(V386))
#endif
#if !defined(USE_TERMIOS)
#define USE_TERMIOS	(defined(HPUX)||defined(SUN5)||defined(PTX)||defined(IRIX5)||defined(IRIX6)||defined(LINUX))
#endif
#if !defined(USE_TCBREAK)
#define USE_TCBREAK	(defined(SUN4)||defined(PTX))
#endif

/* if we have <strings.h> define this to 1, else define to 0
 */
#if !defined(USE_STRINGS)
#define USE_STRINGS	(defined(DYNIX)||defined(EPIX))
#endif

/*
 * if you use MD5 to encrypt passwords, like with FreeBSD
 */
#if !defined(USE_MD5)
# define USE_MD5 defined(FREEBSD)
#endif /* !defined(USE_MD5) */

#if !defined(NEED_UNISTD_H)
#define NEED_UNISTD_H	(defined(SUN5)||defined(PTX)||defined(LINUX))
#endif

/* Some sites set the system hostname to the first part of the FDQN, this
 * is wrong.  If both host.X and other.Y have to use the console server chat
 * protocol to establish a connection.  This because the server running
 * on host.X send "port@host" as the host to connect to, then because
 * other.Y looks up "host.Y" which might be the worng host or just not
 * exist (more likely).  Reported by gregf@sequent.com.  Fix: ksb.
 */
/* Use the name server to make our host into a FDQN.  If -DLOCAL_FDQN_HACK
 * then force this without reguard to system config info
 */
#if !defined(NEED_FDQN_HACK)
#if LOCAL_FDQN_HACK
#define NEED_FDQN_HACK	1
#else
#define NEED_FDQN_HACK	(defined(PTX)||defined(HPUX))
#endif
#endif

/* sun's resolver code lies to FDQN hack and returns the proffered hostname
 * as the canonical host name -- so we lookup the address to force the FDQN.
 */
#if !defined(NEED_REV_FDQN)
#define NEED_REV_FDQN	(defined(SUN5)||defined(SUN4)||defined(SUN3))
#endif

#if !defined(USE_SYS_TIME_H)
#define USE_SYS_TIME_H	(!defined(PTX))
#endif

#if USE_STRINGS
#define	strchr	index
#define strrchr	rindex
#endif

/* used to force the server process to clear parity, which is for farmers
 */
#if !defined(CPARITY)
#define CPARITY		1
#endif


/* if you do not have fd_set's here is a possible emulation
 */
#if USE_OLDSEL
typedef long fd_set;

#define FD_ZERO(a) {*(a)=0;}
#define FD_SET(d,a) {*(a) |= (1 << (d));}
#define FD_CLR(d,a) {*(a) &= ~(1 << (d));}
#define FD_ISSET(d,a) (*(a) & (1 << (d)))
#endif

#if USE_TERMIOS
#if defined(HPUX7)||defined(HPUX8)||defined(HPUX9) 
#define TCGETS  _IOR('T', 16, struct termios)
#define TCSETS  _IOW('T', 17, struct termios)
#endif
#if defined(PTX2)
#define TCGETS  TCGETP
#define TCSETS  TCSETP
#endif
#endif

/* which type does wait(2) take for status location
 */
#if HAVE_UWAIT
#define WAIT_T	union wait
#if ! defined WEXITSTATUS
#define WEXITSTATUS(x)	((x).w_retcode)
#endif
#else
#define WAIT_T	int
#endif

/* which type signal handlers return on this machine
 */
#if !defined(SIGRET_T)
#if defined(sun) || defined(NEXT2) || defined(SUN5) || defined(PTX) || defined(IRIX5)||defined(IRIX6) || defined(IBMR2) || defined(FREEBSD) || defined(ULTRIX) || defined(LINUX) || defined(BSDI) || defined(HPUX10)
#define SIGRET_T	void
#else
#define SIGRET_T	int
#endif
#endif

#if !defined(HAVE_STICKY_SIGNALS)
#define HAVE_STICKY_SIGNALS	(!(defined(HPUX)||defined(SUN5)))
#endif

/* do we have a (working) setsockopt call
 */
#if !defined(HAVE_SETSOCKOPT)
#define HAVE_SETSOCKOPT	(defined(sun)||defined(PTX)||defined(FREEBSD))
#endif

/* does this system have the ANSI strerror() function?
 */
#if !defined(HAVE_STRERROR)
#define HAVE_STRERROR	(defined(IBMR2)||defined(ETA10)||defined(V386)||defined(SUN5)||defined(NEXT2)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10)||defined(HPUX11)||defined(PTX)||defined(IRIX5)||defined(IRIX6)||defined(FREEBSD)||defined(LINUX))
#endif
#if ! HAVE_STRERROR
extern int errno;
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif

#if !defined(HAVE_H_ERRLIST)
#define HAVE_H_ERRLIST  (defined(SUN4)||defined(SUN3)||defined(FREEBSD)|defined(NETBSD)||defined(PTX)||defined(IRIX5)||defined(IRIX6))
#endif
#if HAVE_H_ERRLIST
extern int h_errno;
extern char *h_errlist[];
#define hstrerror(Me)   (h_errlist[Me])
#else
#define hstrerror(Me)   "host lookup error"
#endif

#if !defined(HAVE_RLIMIT)
#if (defined(SUN5)||defined(PTX4))
#define HAVE_RLIMIT	1
#else
#define HAVE_RLIMIT	0
#endif
#endif

#if !defined(HAVE_STDLIB_H)
#define HAVE_STDLIB_H	(defined(IBMR2)||defined(PTX)||defined(FREEBSD)||defined(MSDOS)||defined(BSDI)||defined(NEXT2)||defined(LINUX)||defined(HPUX10))
#endif

#if !defined(HAVE_MALLOC_H)
#define HAVE_MALLOC_H	(defined(PTX)||defined(NETBSD)||defined(IBMR2)||defined(SUN5)||defined(MSDOS)||defined(IRIX)||defined(ULTRIX)||defined(HPUX10))
#endif

#if HAVE_MALLOC_H
#include <malloc.h>
#else
#if NEED_MALLOC_EXTERN
extern char *malloc(), *calloc(), *realloc();
#endif
#endif
