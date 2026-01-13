/*
 * $Id: labstat.h,v 4.0 1994/05/27 13:28:10 ksb Exp $
 *
 * Copyright 1992 Purdue Research Foundation, West Lafayette, Indiana
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

/* some machine specific details
 */
#if !defined(USE_OLDSEL)
#if	defined(IBMR2)
#include <sys/select.h>
#endif
#endif
#if !defined(HAVE_UWAIT)
#define HAVE_UWAIT	!(defined(IBMR2)||defined(SUN5))
#endif

#if !defined(HAVE_WAIT3)
#define HAVE_WAIT3	!defined(SUN5)
#endif

/* This is the port number used in the connection.  It can use either
 * /etc/services or a hardcoded port (SERVICE name has precedence).
 * (You can -D one in the Makefile to override these.)
 */
#if !defined(SERVICE)
#define SERVICE		"labinfo"
#endif

#if !defined(PORT)
#define PORT		785
#endif

#if !defined(HAVE_SETSID)
#define HAVE_SETSID	(defined(IBMR2)||defined(HPUX7)||defined(SUN5))
#endif

/* if we have <strings.h> define this to 1, else define to 0
 */
#if !defined(USE_STRINGS)
#define USE_STRINGS	(defined(DYNIX)||defined(EPIX))
#endif

#if !defined(NEED_UNISTD_H)
#define NEED_UNISTD_H	(defined(SUN5))
#endif

#if USE_STRINGS
#define	strchr	index
#define strrchr	rindex
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

/* does this system have the ANSI strerror() function?
 */
#if !defined(HAVE_STRERROR)
#define HAVE_STRERROR	(defined(IBMR2)||defined(ETA10)||defined(V386)||defined(SUN5)||defined(NEXT2))
#endif
#if ! HAVE_STRERROR
extern int errno;
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif

#if !defined(HAVE_RLIMIT)
#if defined(SUN5)
#define HAVE_RLIMIT	1
#else
#define HAVE_RLIMIT	0
#endif
#endif

#if !defined(HAVE_H_ERRLIST)
#define HAVE_H_ERRLIST  (defined(SUN4)||defined(SUN3)||defined(NETBSD)||defined(IRIX5))
#endif

#if HAVE_H_ERRLIST
extern int h_errno;
extern char *h_errlist[];
#define hstrerror(Me)   (h_errlist[Me])
#else
#define hstrerror(Me)   "host lookup error"
#endif

#if !defined(__STDC__) && !defined(IBMR2)
extern char *calloc(), *malloc(), *realloc();
#endif
