/*
 * $Id: configure.h,v 8.10 2008/03/14 18:03:49 ksb Exp $
 * Copyright 1990 Purdue Research Foundation, West Lafayette, Indiana
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
 **This is a modified version: moved big parts to "machine.h"  -- ksb
 */

/* install includes this file for the system admin to #define things
 * she wants to change from the default values.  Read the descriptions
 * in the install source code (lines 150-250) and #define them here
 * to the local values.
 *
 * Some other major feature can be activated here.
 */

/* the macros {DEF,DEFDIR,ODIR}{MODE,GROUP,OWNER}
 * provide the default modes for any new files installed on the system
 * a value in "quotes" will force that value, a nil pointer will
 * force the value to be inherited from the parrent directory.
 *
 * The ones you do not set will be extrapolated from the others,
 * start with DEFMODE as the default mode for a file, DEFDIRMODE
 * as the defualt directory mode for all directories, and ODIRMODE
 * as the mode for building OLD directories.
 */
#if 0
#define DEFGROUP	"daemon"	/* ls -lg /bin/ls for a guess	*/
#define ODIROWNER	"charon"	/* safe login for purge to use	*/
#endif
#define ODIRGROUP	((char *)0)	/* inherit group so mode works	*/
#define ODIRMODE	((char *)0)	/* inherit the OLD dir mode	*/

#if !defined(DEFGROUP)
#if EPIX || IBMR2 || V386 || SUN5 || PARAGON
#define DEFGROUP	"bin"
#endif
#if SUN4 || NEXT2
#define DEFGROUP	"system"
#endif
#endif

/* this option allows all the binaries in a directory to
 * get their default owner/group/mode from the directory
 * (rather then hard coded defaults)
 * This has the drawback that errors are propogated as well!
 */
#define INHERIT_OK	FALSE		/* propogate modes down tree	*/


/* should the user be able to install a file on top of a symbolic link?
 */
#define DLINKOK		TRUE		/* yes, back it up		*/


/* should the user be able to use a symbolic link as a source file?
 */
#define SLINKOK		TRUE		/* yes, copy as a plain file	*/


/* Here you may #define alternate paths to some programs, because installus
 * runs us with escalated privs we really want full paths here.  --ksb
 * Set any of:  BIN{CHGRP,LS,MKDIR,RANLIB,STRIP}, for example in the
 * makefile use -DBINRANLIB='"/opt/gnu/bin/ranlib"'
 */

#if !defined(BINRANLIB) && defined(NEXT2)
#define BINRANLIB	"/bin/ranlib"		/* The NeXTs have it in /bin */
#endif
#if !defined(BINCHGRP) && (defined(FREEBSD) || defined(SUN5))
#define BINCHGRP	"/usr/bin/chgrp"	/* free bsd is in /usr/bin */
#endif
#if !defined(BINSTRIP)
#if defined(FREEBSD) || defined(NETBSD) || defined(OPENBSD) || defined(LINUX) || defined(BSDI)
#define BINSTRIP	"/usr/bin/strip"	/* free bsd is in /usr/bin */
#endif
#if defined(SUN5)
#define BINSTRIP	"/usr/ccs/bin/strip"	/* hard to believe this	*/
#endif
#endif


/* here you may set the flags used when we (under -v) run ls on a file
 * or directory
 *	LS{,DIR}ARGS
 */
#if !defined(LSARGS)
#if defined(SUN5)
#define LSARGS          "-ls"
#else
#define LSARGS		"-lsg"
#endif
#endif


/* CONFIG controls the code to consult a checklist of special files.
 * Set CONFIG to "", or (char *)0, for other effects.
 * By #define'ing a checklist file you enable the code for it.
 * Warning -DCONFIG sets it to `1' which breaks the C code.
 *	-DCONFIG='""'	no list, but include the check code
 *	-UCONFIG		no check and no check code
 */
#if !defined(CONFIG)
#define CONFIG "/usr/local/lib/install.cf"	/* check list, and code	*/
#endif


/* INST_FACILITY controls installation syslogging.
 * (LOG_LOCAL2 was the first local slot we had a PUCC)
 * To enable installation syslog'ing uncomment the #define below.
 */
#if defined(bsd)||defined(HPUX)||defined(SUN3)||defined(SUN4)||defined(SUN5)||defined(IBMR2)

#if ! defined(INST_FACILITY)
#if defined(LOG_LOCAL2)
#define INST_FACILITY LOG_LOCAL2 	/* our 4.3 log facility		*/
#else
#define INST_FACILITY LOG_INFO 		/* our 4.2 log facility		*/
#endif
#endif /* given on the command line, keep it */

#else /* not bsd, not dynix */

#if defined(ETA10)||defined(V386)
#if !defined(INST_FACILITY)
#define INST_FACILITY LOG_LOCAL2 	/* ETA10 uses 4.3 log facility	*/
#endif /* given on the command line, keep it */
#else	/* assume no logging here */
#undef INST_FACILITY			/* we do not log changes	*/
#endif

#endif
