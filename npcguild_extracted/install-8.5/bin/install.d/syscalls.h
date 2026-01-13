/*
 * $Id: syscalls.h,v 8.0 1996/03/13 15:44:36 kb207252 Dist $
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
extern void BlockSigs(void);
extern void UnBlockSigs(void);
extern void Die(char *);
extern struct group * savegrent(struct group *);
extern struct passwd *savepwent(struct passwd *);
extern void ChMode(char *, int);
extern void ChGroup(char *, struct group *pgroup);
extern void ChOwnGrp(char *, struct passwd *, struct group *);
extern void ChTimeStamp(char *, struct stat *);
extern int DoCopy(char *, char *);
extern char *Mytemp(char *);
extern void MungName(char *);
extern char *StrTail(char *);
#if HAVE_SLINK
extern int CopySLink(char *, char *);
#endif	/* don't handle links at all */
extern int Rename(char *, char *, char *);
extern int IsDir(char *);
extern int MyWait(int);
extern int RunCmd(char *, char *, char *);
#else
extern void BlockSigs();
extern void UnBlockSigs();
extern void Die();
extern struct group * savegrent();
extern struct passwd *savepwent();
extern void ChMode();
extern void ChGroup();
extern void ChOwnGrp();
extern void ChTimeStamp();
extern char *Mytemp();
extern void MungName();
extern char *StrTail();
extern int DoCopy();
#if HAVE_SLINK
extern int CopySLink();
#endif	/* don't handle links at all */
extern int Rename();
extern int IsDir();
extern int MyWait();
extern int RunCmd();
#endif

extern uid_t geteuid();
extern gid_t getegid();
