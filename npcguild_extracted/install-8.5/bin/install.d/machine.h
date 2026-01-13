/*
 * $Id: machine.h,v 8.10 2000/01/31 00:59:29 ksb Exp $
 * Copyright 1997 Kevin S Braunsdorf. All rights reserved.
 *
 * Written by Kevin S Braunsdorf, ksb@fedex.com
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
#if !defined(HPUX) && (defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10)||defined(HPUX11))
#define HPUX	1
#endif

#if !defined(IRIX) && (defined(IRIX4) || defined(IRIX5) || defined(IRIX6))
#define IRIX	1
#endif

#if !defined(ULTRIX) && (defined(ULTRIX4) || defined(ULTRIX5))
#define ULTRIX	1
#endif

/* if we still don't know the platform look for other clues
 */
#if !defined(bsd) && !defined(SYSV) && !defined(DYNIX) && !defined(HPUX7) && !defined(HPUX8) && !defined(HPUX9) && !defined(HPUX10) && !defined(HPUX11) && !defined(SUN3) && !defined(SUN4) && !defined(IBMR2) && !defined(ETA10) && !defined(V386) && !defined(ULTRIX)

#if defined(BSD)
#define bsd 1
#else
#if defined(dynix)
#define DYNIX 1
#endif /* try dynix */
#endif /* try bsd */

#endif /* still do not know */

/* DYNIX looks enough like bsd for this to be true
 */
#if (defined(DYNIX) || defined(EPIX)) && !defined(bsd)
#define bsd
#endif

/* the ETA10 looks enough like a sys5 machine for this, mostly
 */
#if (defined(V386) || defined(ETA10)) && !defined(SYSV)
#define SYSV	1
#endif

#if defined(IBMR2)
#undef _ANSI_C_SOURCE
#define _ANSI_C_SOURCE	1
#endif


#if !defined(HAVE_STRERROR)
#if PTX||PARAGON||MSDOS
#define HAVE_STRERROR	1
#else
#define HAVE_STRERROR	(defined(NEXT2)||defined(IBMR2)||defined(HPUX)||defined(SUN5)||defined(NETBSD)||defined(IRIX5)||defined(IRIX6)||defined(FREEBSD)||defined(ULTRIX)||defined(BSDI)||defined(LINUX))
#endif
#endif


#if HAVE_SIMPLE_ERRNO
extern int errno;
#endif

#if !HAVE_STRERROR
#if !defined(strerrror)
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif
#endif

/* Ulrtix4 const parameters are not allowed, here I refrain from comment.
 */
#if !defined(BROKEN_CONST)
#define BROKEN_CONST		defined(ULTRIX4)
#endif

#if BROKEN_CONST
#define	const		/* broken -- removed */
#endif

#if !defined(HAVE_STRDUP)
#define HAVE_STRDUP	(!ULTRIX)
#endif

#if !HAVE_STRDUP
#define strdup(Mp)	(strcpy(malloc((strlen(Mp)|7)+1),Mp))
#endif


/* do we have symbolic links (#inlcude stat.h for best results)
 */
#if !defined(HAVE_SYMLINKS)
#define HAVE_SYMLINKS	(defined(S_IFLNK)&&!defined(V386))
#endif
#if !defined(S_IXUSR)
#define S_IXUSR	S_IEXEC
#endif
#if !defined(S_IXGRP)
#define S_IXGRP	(S_IEXEC>>3)
#endif
#if !defined(S_IXOTH)
#define S_IXOTH	(S_IEXEC>>6)
#endif

#if !defined(HAVE_CSYMLINKS)
#define HAVE_CSYMLINKS	(defined(DYNIX)||defined(S81))
#endif

/* if we have ansi C prototypes (cannot depend on __STDC__, sorry)
 */
#if !defined(HAVE_PROTO)
#define HAVE_PROTO	0
#endif


/* do we have mkdir(2) and rename(2) system calls?
 */
#if !defined(HAVE_MKDIR)
#define HAVE_MKDIR	!(defined(pdp11) || defined(SYSV))
#endif

#if !defined(HAVE_RENAME)
#define HAVE_RENAME	HAVE_MKDIR
#endif

/* short names are likely to break some instck checks, BTW.
 */
#if !defined(HAVE_SHORTNAMES)
#define HAVE_SHORTNAMES	(defined(pdp11) || defined(SYSV))
#endif


/* do we have setsigmask?
 */
#if !defined(HAVE_SIGMASK)
#define HAVE_SIGMASK	((!defined(SUN5)&&defined(bsd))||defined(IBMR2))
#endif


/* do we have the statfs call to check for NFS file systems?
 */
#if !defined(HAVE_STATFS)
#define HAVE_STATFS	(defined(ETA10)||defined(V386)||defined(SUN5))
#endif


/* do we have struct timeval?
 */
#if !defined(HAVE_TIMEVAL)
#define HAVE_TIMEVAL	!(defined(HPUX)||defined(SYSV))
#endif


/* do we have to run a ranlib command on *.a files (-l)
 */
#if !defined(HAVE_RANLIB)
#define HAVE_RANLIB	!(defined(SYSV)||defined(HPUX)||defined(EPIX)||defined(SUN5)||defined(PARAGON)||defined(LINUX))
#endif

#if !defined(USE_ELF)
#define USE_ELF		(defined(SUN5)||defined(IRIX))
#endif

#if !defined(USE_FILEHDR)
#define USE_FILEHDR	(defined(HPUX9)||defined(HPUX10)||defined(HPUX11))
#endif

#if !defined(HAVE_RANLIB_H)
#define HAVE_RANLIB_H	(HAVE_RANLIB && !(defined(IBMR2) || defined(SUN5) || defined(IRIX)))
#endif

/* the __symdef file in a ranlib'd archive is a clue on some systems
 */
#if !defined(HAVE_SYMDEF)
#define HAVE_SYMDEF	defined(bsd)
#endif

#if !defined(HAVE_SGSPARAM)
#define HAVE_SGSPARAM	(defined(SYSV)&&!(defined(V386)||defined(EPIX)))
#endif


/* which file to include <strings.h> or <string.h> {STRINGS=1, STRING=0}
 */
#if !defined(STRINGS)
#define STRINGS		(!defined(SUN5)&&defined(bsd))
#endif

/* which file to include <sys/signal.h> or <signal.h>
 */
#if !defined(SIGNAL_IN_SYS)
#define	SIGNAL_IN_SYS	defined(IBMR2)
#endif

/* where do we include fcntl.h from?
 */
#if !defined(FCNTL_IN_SYS)
#define FCNTL_IN_SYS	(defined(SYSV)||defined(HPUX7)||defined(HPUX8))
#endif

/* checklist or mtab
 */
#if !defined(USE_CHECKLIST)
#define USE_CHECKLIST	(defined(HPUX7)||defined(HPUX8)||defined(HPUX9))
#endif

#if !defined(USE_MNTENT)
#define USE_MNTENT	(defined(IRIX)||defined(HPUX10)||defined(HPUX11)||defined(LINUX))
#endif

#if !defined(USE_VFSENT)
#define USE_VFSENT	(defined(SUN5))
#endif


/* do we have scandir,readdir and friends in <sys/dir.h>, else look for
 * <ndir.h>
 */
#if !defined(HAS_NDIR)
#define HAS_NDIR        (defined(ETA10)||defined(V386))
#endif

#if !defined(HAS_DIRENT)
#define HAS_DIRENT      (defined(SUN5)||defined(LINUX))
#endif

/* if we have BSDDIR which name is the type? (struct direct) or (struct dirent)
 */
#if !defined(DIRENT)
#if defined(IBMR2) || defined(SUN5) || defined(PARAGON) || defined(LINUX)
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

/* do we have a (union wait) type?
 */
#if !defined(HAVE_UWAIT)
#define	HAVE_UWAIT	(defined(SUN3)||defined(SUN4)||(!defined(SUN5)&&!defined(PARAGON)&&defined(bsd)))
#endif


/* do we have the (kinda) new open(pc, how, mode)?
 */
#if !defined(HAVE_OPEN3)
#define HAVE_OPEN3	(defined(bsd)||defined(SUN3)||defined(SUN4)||defined(IBMR2))
#endif

/* do we have accounting type (uid_t, gid_t)?  (default to int)
 */
#if defined(pdp11) || defined(DYNIX) || defined(SYSV) || defined(SUN3)||defined(SUN4)
#define HAVE_ACCTTYPES	0
#else
#define HAVE_ACCTTYPES	1
#endif


/* do we have a comment field in the (struct passwd) type?
 */
#if !defined(HAVE_PWCOMMENT)
#define HAVE_PWCOMMENT	(!defined(IBMR2)&&!defined(NETBSD)&&!defined(FREEBSD)&&!defined(SUN5)&&!defined(LINUX)&&!defined(BSDI))
#endif


/* do we have a kernel quota system?  (bsd like)
 * (if so savepwent has to copy a quota struct)
 */
#if !defined(HAVE_QUOTA)
#define HAVE_QUOTA	(defined(bsd)&&!defined(NETBSD)&&!defined(FREEBSD)&&!defined(BSDI))
#endif


/* if your chmod(2) will not drop setgid bits on a dir with `chmod 755 dir`
 * you need to define BROKEN_CHMOD (Suns have this problem -- yucko!)
 * See SunBotch() in instck.c for more flamage.
 */
#if !defined(BROKEN_CHMOD)
#define BROKEN_CHMOD	(defined(SUN3)||defined(SUN4))
#endif


/* instck needs to know the name of the magic number field in a (struct exec)
 * in <a.out.h> or <sys/exec.h>, and some other stuff like that
 */
#if defined(NETBSD)
#define	MNUMBER(Mex)	N_GETMAGIC(Mex)
#define	MSYMS		a_syms
#define EXHDR		struct exec
#else
#if defined(SUN5)||defined(BSDI)
#define MNUMBER(Mex)	(Mex).a_magic
#define MSYMS		a_syms
#define EXHDR		struct exec
#define USE_SYS_EXECHDR_H	defined(SUN5)
#else
#if defined(EPIX) || defined(PARAGON)
#define MNUMBER(Mex)	(Mex).magic
#define MSYMS		magic
#define EXHDR		struct aouthdr
#else
#if USE_FILEHDR
#define MNUMBER(Mex)	(Mex).a_magic
#define MSYMS		symbol_total
#define EXHDR		struct header
#else
#if defined(HPUX7)||defined(HPUX8)
#define MNUMBER(Mex)	(Mex).a_magic.file_type
#define MSYMS		a_lesyms
#define EXHDR		struct exec
#else
#if defined(bsd)||defined(SUN3)||defined(SUN4)
#define MNUMBER(Mex)	(Mex).a_magic	/* magic number			*/
#define MSYMS		a_syms		/* number of symbols (strip)	*/
#define EXHDR		struct exec	/* type of this struct		*/
#else
#if defined(ETA10)||defined(IBMR2)||defined(V386)||defined(ULTRIX)
#define MNUMBER(Mex)	(Mex).f_magic
#define MSYMS		f_nsyms
#define EXHDR		struct filehdr
#else /* SYSV? */
#define MNUMBER(Mex)	(Mex).a_info
#define MSYMS		a_syms
#define EXHDR		struct exec
#endif
#endif
#endif
#endif
#endif
#endif
#endif

#if !defined(USE_STDLIB)
#if IBMR2||PTX||FREEBSD||MSDOS||ULTRIX||BSDI
#define USE_STDLIB	1
#else
#define USE_STDLIB	0
#endif
#endif

#if !defined(USE_MALLOC_H)
#define USE_MALLOC_H	(defined(PTX)||defined(NETBSD)||defined(IBMR2)||defined(SUN5)||defined(MSDOS)||defined(IRIX)||defined(ULTRIX)||defined(HPUX11))
#endif

#if !defined(NEED_MALLOC_EXTERN)
#define NEED_MALLOC_EXTERN	(!USE_MALLOC_H && !defined(NEXT2) && !defined(IRIX) && !defined(BSDI))
#endif

/* find the getwd/getcwd call that works
 * Some sysV machines have getwd, some getcwd, some none
 * If you do not have one, get one.
 * (maybe popen a /bin/pwd and read it into the buffer?)
 */
#if !defined(HAVE_GETCWD)
#if defined(SYSV) || (defined(HPUX) && !defined(HPUX10)) || defined(IBMR2) || defined(SUN5)
#define HAVE_GETCWD	1
#else
#define HAVE_GETCWD	0
#endif
#endif
#if !defined(HAVE_GETWD)
#define HAVE_GETWD	(!HAVE_GETCWD)
#endif

#if !HAVE_GETWD
#define getwd(Mp) getcwd(Mp,MAXPATHLEN)	/* return pwd		*/
#endif

#if USE_STDLIB || defined(__STDC__)
#include <stdlib.h>
#endif

#if USE_MALLOC_H
#include <malloc.h>
#else
#if NEED_MALLOC_EXTERN
extern char *malloc(), *calloc(), *realloc();
#endif
#endif

#if USE_STDLIB || defined(__STDC__)
#include <stdlib.h>
#endif

#if USE_UNISTD_H
#include <unistd.h>
#else
#if HAVE_GETWD
extern char *getwd();
#endif
#endif
