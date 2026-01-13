/*
 * $Id: file.h,v 8.0 1996/03/13 15:44:36 kb207252 Dist $
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

#if HAVE_PROTO
extern void MkOld(char *);
extern int DoBackup(int, char *, char *, char *);
extern void MakeNames(char *, char *, char *, char *);
extern int DoLinks(struct stat *, char *, char *, int, struct passwd *, struct group *);
extern int LaunchLinks(struct stat *, char *, char *, char *, int, struct passwd *, struct group *);
extern int Install(char *, char *, char *, char *);
#else
extern void MkOld();
extern int DoBackup();
extern void MakeNames();
extern int DoLinks();
extern int LaunchLinks();
extern int Install();
#endif
