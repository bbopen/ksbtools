/*
 * $Id: special.h,v 8.2 2009/12/08 22:58:14 ksb Exp $
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
 * if we have a config file (check list) we need to represent each	(ksb)
 * line of the file in a struct.
 */
#if defined(CONFIG)

#define	CF_STRIP	's'
#define CF_RANLIB	'l'
#define CF_NONE		'n'
#define CF_ANY		'?'

#define CF_IS_STRIP(MCF)	(CF_STRIP == (MCF).chstrip)
#define CF_IS_RANLIB(MCF)	(CF_RANLIB == (MCF).chstrip)
#define CF_IS_NONE(MCF)		(CF_NONE == (MCF).chstrip)
#define CF_IS_ANY(MCF)		(CF_ANY == (MCF).chstrip)

typedef struct CFnode {
	int ffound;		/* is the file we have			*/
	int mtype;		/* file type bits			*/
	int mmust;		/* mode as an int			*/
	int moptional;		/* optional mode as an int		*/
	short int fbangowner;	/* !root form for owner			*/
	short int fbanggroup;	/* !system form for group		*/
	char acmode[16];	/* mode as a string			*/
	char acowner[16];	/* owner as a string			*/
	char acgroup[16];	/* group as a string			*/
	char chstrip;		/* strip ('l','s','-','*')		*/
	char *pcflags;		/* chflags options, abutted to strip	*/
	char *pcmesg;		/* text to print after install		*/
	uid_t uid;		/* used only by instck, to save lookups */
	gid_t gid;		/* same as above			*/
	int iline;		/* the matching line on the cf file	*/
	char *pcspecial;	/* the special file we were searching	*/
	char *pcpat;		/* the glob pattern we matched on	*/
	char *pclink;		/* we are just a link to another file	*/
} CHLIST;

#if !defined(MAXCNFLINE)
#define MAXCNFLINE	512
#endif	/* length of a config line		*/

#if HAVE_PROTO
extern char *NodeType(unsigned int, char *);
extern void ModetoStr(char *, int, int);
extern int CheckFFlags(const char *, unsigned long, unsigned long , unsigned long, unsigned long );
#if defined(INSTCK)
extern void DirCk(int, char **, char *, CHLIST *);
#endif
extern void Special(char *, char *, CHLIST *);

#else
extern char *NodeType();
extern void ModetoStr();
extern int CheckFFlags();
#if defined(INSTCK)
extern void DirCk();
#endif
extern void Special();

#endif

#endif	/* set check list flags from config file */

#define IS_LINK(MCL)  ((char *)0 != (MCL).pclink)
extern char *apcOO[];		/* name of 0/1 for user			*/
extern struct group *grpDef;	/* default group on all files (this dir)*/
extern struct passwd *pwdDef;	/* default owner on all files (this dir)*/

/*
 * this macro could be in glob.h, or not...
 */
#define SamePath(Mglob, Mfile)	_SamePath(Mglob, Mfile, 1)

#if HAVE_PROTO
extern void CvtMode(char *, int *, int *);
extern int _SamePath(char *, char *, int);
extern void InitCfg(char *, char *);
#else
extern void CvtMode();
extern int _SamePath();
extern void InitCfg();
#endif
