/*
 * $Id: instck.h,v 8.3 2009/09/30 16:03:00 ksb Exp $
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

#undef PATCH_LEVEL
#define PATCH_LEVEL	1

/*
 * configure the maxfreq routines for instck				(ksb)
 */

typedef struct PDnode {
	short fseen;
} PATH_DATA;
extern void PUInit();

#if !defined(minor)
#define minor(Md)	((int)(((unsigned)(Md))&0xffff))
#endif

#if !defined(major)
#define major(Md)	((int)(((unsigned)(Md)>>16)&0xffff))
#endif

#if !defined(MAXANS)
#define MAXANS		32		/* answer for QExec prompt	*/
#endif


extern CHLIST CLCheck;
extern int iMatches;

#define fpOut		stdout		/* so we could make it a file	*/

#define QE_SKIP		's'
#define QE_RAN		'r'
#define QE_NEXT		'n'

#if !defined(WAIT_T)
#if HAVE_UWAIT
#define WAIT_T union wait
#else
#define WAIT_T int
#endif
#endif	/* return value from wait */

#if HAVE_PROTO
extern int ElimDups(int, char **);
extern void NoMatches(char *);
extern void BadSet(int, int, char *, char *);
extern int FileMatch(char *, char *, int (*)());
extern int DoCk(char *, struct stat *);
extern void InstCk(int, char **, char *, CHLIST *);
extern int QExec(char **, char *, WAIT_T *);
#else
extern int ElimDups();
extern void NoMatches();
extern void BadSet();
extern int FileMatch();
extern int DoCk();
extern void InstCk();
extern int QExec();
#endif

extern char *apcCmp[], *apcRm[], *apcRmdir[], *apcInstall[], *apcLn[], *apcMv[], *apcChmod[];
