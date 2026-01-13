/*
 * abbreviation fucntions, maintain a unique list of strings that are
 * compressed
 *
 * $Id: abrv.h,v 4.0 1996/03/19 15:18:49 kb207252 Beta $
 * 
 * include srtunq.h first
 */

#define MXABR 26		/* upper case chars used		*/
extern char *abrvtbl[];		/* translation table strings		*/
extern int abrvlen[];		/* string lengths (for speed)		*/
extern struct srtent abrv;	/* include file abrevs			*/

extern int lngsrt();		/* compare function for abrevs		*/
extern char *hincl();		/* optimizer for include paths		*/
extern void srchincl();		/* find [A-Z] makefile defines		*/
extern void abrvsetup();	/* create abrvs, write them		*/
extern int findabr();		/* find longest abrv			*/
