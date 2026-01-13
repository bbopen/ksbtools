/*
 * $Id: main.h,v 8.1 1997/07/30 14:33:59 ksb Exp $
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
 */


/* global variables for options
 */
extern char *progname;		/* tail of argv[0]			*/
extern char *Group;		/* given group ownership		*/
extern char *Owner;		/* given owner				*/
extern char *Mode;		/* given mode				*/
extern char *HardLinks;		/* hard links to installed file		*/
extern char *SoftLinks;		/* symlinks to installed file		*/
extern int Copy;		/* we're copying			*/
extern int Destroy;		/* user doesn't want an OLD		*/
extern int BuildDir;		/* we're installing a directory		*/
extern int KeepTimeStamp;	/* if preserving timestamp		*/
extern int fQuiet;		/* suppress some errors, be quiet	*/
extern int Ranlib;		/* if we're running ranlib(1)		*/
extern int Strip;		/* if we're running strip(1)		*/
extern int fDelete;		/* remove the file/dir, log it		*/
extern int fRecurse;		/* Sun-like recursive dir install	*/
extern int fTrace;		/* if just tracing			*/
extern int fVerbose;		/* if verbose				*/
extern int f1Copy;		/* only keep one copy			*/
extern int fOnlyNew;		/* new install only - abort when file -f*/
#if defined(CONFIG)
extern struct passwd *pwdDef;	/* aux default owner for config file	*/
extern struct group *grpDef;	/* aux default group for config file	*/
extern char *pcSpecial;		/* file contains ckecked paths		*/
#endif	/* have a -C option?			*/

/*
 * global variables, but not option flags
 */
extern int bHaveRoot;		/* we have root permissions		*/
extern char *pcGuilty;		/* the name logged as the installer	*/

#define INSTALL	1
