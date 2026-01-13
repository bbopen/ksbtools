/*
 * $Id: gen.h,v 8.1 1999/02/17 01:48:55 ksb Exp $
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

/*
 * configure parts we need (maxfreq) and export our abstraction		(ksb)
 */
#define MAXGROUPNAME	32	/* size for a group name		*/

typedef struct MEnode {
	int iuid;
	int igid;
	int imode;
	char chstrip, chtype;
	char *pcext;
} ME_ELEMENT;


#if HAVE_PROTO
extern int ListPref(char **, char *pcLeft, char *pcRight);
extern int GenCk(int, char **);
extern int MECompare(ME_ELEMENT *, ME_ELEMENT *);
extern int MECopy(ME_ELEMENT *, ME_ELEMENT *);
#else
extern int ListPref();
extern int GenCk();
extern int MECompare();
extern int MECopy();
#endif
