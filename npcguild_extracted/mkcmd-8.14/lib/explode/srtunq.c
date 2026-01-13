/*@Version $Revision: 6.0 $@*/
/*@Header@*/
/* Sorting function that inserts strings one at a time into memory.
 * Strings are null terminated.
 * Only uniq strings are stored (no count is kept of how many)
 * Any memory used is freed on init (or re-init).
 *
 * Author: Steve Uitti, PUCC, 1985
 *
 * $Log: srtunq.c,v $
 * Revision 6.0  2000/07/30 15:57:11  ksb
 * uprev for release
 *
 * Revision 5.3  2000/07/30 15:52:01  ksb
 * extra :
 * /
 *
 * Revision 5.2  1997/01/09 05:00:29  kb207252
 * removed unused variables and made progname in the right scope
 *
 * Revision 5.1  1997/01/09  03:32:12  kb207252
 * first release with mkcmd
 *
 * Revision 1.1  1997/01/07  13:24:01  kb207252
 * Initial revision
 *
 * Revision 2.0  90/06/01  14:30:07  ksb
 * net release June 90
 * 
 * Revision 1.1  90/04/13  16:09:57  ksb
 * Initial revision
 * 
 * Revision 1.1  87/12/09  11:11:28  ksb
 * Initial revision
 * 
 * Revision 1.3  87/12/07  15:58:29  ksb
 * typedef names and a mainline test driver
 * 
 * Revision 1.2  87/11/12  14:39:33  ksb
 * set ENOMEM in srtin
 * 
 * Revision 1.1  87/11/11  15:54:26  ksb
 * Initial revision
 */
#include <stdio.h>		/* for NULL				*/
#include <sys/types.h>		/* for void, in V7			*/
#include "srtunq.h"		/* for srtunq structs & functions	*/

/*@Explode apply@*/
/*
 * apply given function to each string until one returns non-zero
 * return first non-zero (and stop), or 0 for "no match"
 */
int
srtapply(ent, fnc)
SRTTABLE *ent;
int (*fnc)();
{
	register SRTENTRY *s, *top;

	if (0 == (top = s = ent->srt_top)) {
		return 0;
	}
	while (s->srt_less != NULL) {
		s = s->srt_less;
	}

	for (;;) {
		register int i;
		if (0 != (i = (*fnc)(s->srt_str))) {
			return i;
		}
		if (s->srt_more != NULL) {
			s = s->srt_more;
			while (s->srt_less != NULL)
				s = s->srt_less;
			continue;
		}
		for (;;) {
			register SRTENTRY *q;
			if (s == top)
				return 0;
			q = s;
			s = s->srt_prev;
			if (q == s->srt_less)
				break;
		}
	}
}

/*@Explode del@*/
#include <errno.h>
extern int errno, strcmp();
extern char *malloc(), *strcpy();

/*
 * srtdel - delete string in the sorted & unique list.
 * returns one if found, else 0
 */
int
srtdel(ent, str, compar)
SRTTABLE *ent;
char *str;
int (*compar)();
{
	register int i;			/* string compare result	*/
	register SRTENTRY *s, *p, *os;	/* temp				*/
	register SRTENTRY **pps;	/* where			*/

	pps = & ent->srt_top;
	os = NULL;
	while (NULL != (s = *pps)) {
		if (0 == (i = (*compar)(str, s->srt_str)))
			break;
		os = s;
		pps = (i > 0) ? & s->srt_more : & s->srt_less;
	}

	if (NULL == s)
		return 0;

	if (s == ent->srt_next) {	/* if we are the cur, mode cur	*/
		p = s;
		if (p->srt_more != NULL) {
			p = p->srt_more;	/* go one more */
			while (p->srt_less != NULL)/* then all the way less */
				p = p->srt_less;
		} else {
			while (p != ent->srt_top && p->srt_prev->srt_more == p)
				p = p->srt_prev;
			p = (p == ent->srt_top) ? NULL : p->srt_prev;
		}
		ent->srt_next = p;
	}

	p = s->srt_more;
	s = s->srt_less;

	if (os != (*pps)->srt_prev)	/*ZZZ sanity check message please*/
		abort();

	free(*pps);
	for (;;) {
		if (NULL == s) {		/* just one kid		*/
			*pps = p;
			if (NULL == p)
				break;
			p->srt_prev = os;
			break;
		}
		if (NULL == p) {
			*pps = s;
			s->srt_prev = os;
			break;
		}

		if (NULL == p->srt_less) {	/* hand off the middle	*/
			p->srt_prev = os;
			*pps = p;
			p->srt_less = s;
			s->srt_prev = p;
			break;
		}
		if (NULL == s->srt_more) {
			*pps = s;
			s->srt_prev = os;
			s->srt_more = p;
			p->srt_prev = s;
			break;
		}

		if (NULL == s->srt_less) {	/* rotate into middle	*/
			*pps = s->srt_more;
			s->srt_more->srt_prev = os;
			s->srt_more = (*pps)->srt_less;
			s->srt_more->srt_prev = s;
			(*pps)->srt_less = s;
			s->srt_prev = *pps;
			pps = & (*pps)->srt_more;
			s = *pps;
			continue;
		}
		*pps = p->srt_less;
		p->srt_less->srt_prev = os;
		p->srt_less = (*pps)->srt_more;
		p->srt_less->srt_prev = p;
		(*pps)->srt_more = p;
		p->srt_prev = *pps;
		pps = & (*pps)->srt_less;
		p = *pps;
	}
	return 1;
}
/*@Explode dtree@*/
/* srtdtree - recursive tree delete
 * frees all less & more entries pointed to by the srt struct
 */
void
srtdtree(srt)
register SRTENTRY *srt;
{
	if (srt->srt_less != NULL) {
		srtdtree(srt->srt_less);
		free((char *) srt->srt_less);
		srt->srt_less = NULL;
	}
	if (srt->srt_more != NULL) {
		srtdtree(srt->srt_more);
		free((char *) srt->srt_more);
		srt->srt_more = NULL;
	}
}
/*@Explode free@*/
/*
 * srtfree - delete all the data, init the tag
 * is the structure empty?
 */
void
srtfree(ent)
SRTTABLE *ent;
{
	if (ent->srt_top != NULL) {
		srtdtree(ent->srt_top);
		free((char *) ent->srt_top);
		srtinit(ent);
	}
}
/*@Explode gets@*/
/*
 * srtgets - get next string from sorted list, NULL if none more.
 */
char *
srtgets(ent)
SRTTABLE *ent;
{
	register SRTENTRY *s;	/* tmp		*/
	register char *p;	/* ret val		*/

	if ((s = ent->srt_next) == NULL)
		return NULL;

	p = s->srt_str;
	if (s->srt_more != NULL) {
		s = s->srt_more;	/* go one more */
		while (s->srt_less != NULL)/* then all the way less */
			s = s->srt_less;
		ent->srt_next = s;
		return p;
	}
	while (s != ent->srt_top && s->srt_prev->srt_more == s)
		s = s->srt_prev;	/* back down any more's */
	s = (s == ent->srt_top) ? NULL : s->srt_prev;
	ent->srt_next = s;

	return p;
}
/*@Explode gti@*/
/*
 * srtgti - init get string function
 */
void
srtgti(ent)
SRTTABLE *ent;
{
	register SRTENTRY *pSE;

	if (NULL != (pSE = ent->srt_top)) {
		while (pSE->srt_less != NULL)
			pSE = pSE->srt_less;
	}
	ent->srt_next = pSE;
}
/*@Explode in@*/
#include <errno.h>
extern int errno, strcmp();
extern char *malloc(), *strcpy();
/*
 * srtin - insert string in the sorted & unique list.
 * returns the string, else (char *)0 and sets errno
 */
char *
srtin(ent, str, compar)
SRTTABLE *ent;
char *str;
int (*compar)();
{
	register char *p;		/* temp string pointer		*/
	register int i;			/* string compare result	*/
	register SRTENTRY *s, *os;	/* temp				*/
	register SRTENTRY **pps;	/* where			*/

	pps = & ent->srt_top;
	os = NULL;
	while (NULL != (s = *pps)) {
		if (0 == (i = (*compar)(str, s->srt_str)))
			return s->srt_str;
		os = s;
		pps = (i > 0) ? & s->srt_more : & s->srt_less;
	}
	if (NULL == (p = malloc((unsigned) (strlen(str) + sizeof(SRTENTRY))))) {
		errno = ENOMEM;
		return (char *)0;
	}
	s = (SRTENTRY *) p;

	*pps = s;
	s->srt_prev = os;
	s->srt_less = s->srt_more = NULL;

	return strcpy(s->srt_str, str);
}
/*@Explode init@*/
/* srtinit - init the database tags
 * erase any knowledge, extractions at the begining
 */
void
srtinit(ent)
SRTTABLE *ent;
{
	ent->srt_top = ent->srt_next = NULL;
}
/*@Explode mem@*/
#include <errno.h>
extern int errno, strcmp();
extern char *malloc(), *strcpy();

/*
 * srtmem - is a string in the sorted & unique list.
 * returns the string, else (char *)0
 */
char *
srtmem(ent, str, compar)
SRTTABLE *ent;
char *str;
int (*compar)();
{
	register int i;			/* string compare result	*/
	register SRTENTRY *s;		/* temp				*/
	register SRTENTRY **pps;	/* where			*/

	pps = & ent->srt_top;
	while (NULL != (s = *pps)) {
		if (0 == (i = (*compar)(str, s->srt_str)))
			return s->srt_str;
		pps = (i > 0) ? & s->srt_more : & s->srt_less;
	}
	errno = ENOENT;
	return (char *)0;
}

/*@Remove@*/
#if TEST

/*@Explode main@*/
char *progname = "srtunq-test";

int
main(argc, argv)
int argc;
char **argv;
{
	fprintf(stderr, "%s: none anymore, got removed in a botch\n", progname);
	exit(0);
}
/*@Remove@*/
#endif
