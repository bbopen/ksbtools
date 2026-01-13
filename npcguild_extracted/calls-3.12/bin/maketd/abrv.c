/*
 * $Id: abrv.c,v 4.5 1995/04/20 15:15:44 kb207252 Beta $
 *
 * abbreviation related routines
 * Written & hacked by Stephen Uitti, PUCC staff (su)
 * 1985 maketd is copyright (C) Purdue University, 1985
 *
 * Permission is hereby given for its free reproduction and modification for
 * non-commercial purposes, provided that this notice and all embedded
 * copyright notices be retained. Commercial organizations may give away
 * copies as part of their systems provided that they do so without charge,
 * and that they acknowledge the source of the software.
 */
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>

#include "machine.h"
#include "srtunq.h"
#include "abrv.h"
#include "main.h"
#include "maketd.h"

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#define SPACE	'\040'		/* ascii for space			*/

struct srtent abrv;		/* include file abrevs			*/
char   *abrvtbl[MXABR];		/* translation table strings		*/
int     abrvlen[MXABR];		/* string lengths (for speed)		*/

/* lngsrt - string length more important than lexicographical compare.	(su)
 * return > 0 if b is longer than a.
 * return < 0 if b is shorter than a.
 * if a & b are * the same length, return strcmp(a, b), which means that
 * 0 is returned if the strings are THE SAME,
 * if b > a: return > 0 if b < a: return < 0
 */
int
lngsrt(a, b)
char   *a, *b;
{
	register int i;

	if (0 != (i = strlen(b) - strlen(a)))
		return i;
	return strcmp(a, b);
}

/* include header optimizer:						(su)
 * Compress multiple leading /'s to just one. Remove leading "./".
 * Doesn't change data, just returns pointer into beginning of path.
 */
char *
hincl(p)
register char *p;
{
	if ('/' == *p) {		/* compress multiple leading /'s */
		while ('/' == p[1])	/* to just one */
			++p;
	}
	while (0 == strncmp("./", p, 2)) {
		p += 2;			/* leading "./" can confuse make */
		while ('/' == *p)	/* don't change ".//a.h" to "/a.h" */
			++p;
	}
	return p;
}

/* add an abreviation to the table				    (su/ksb)
 */
void
makeabrv(pos, p)
int pos;
char *p;
{
	register int len;

	if (NULL != abrvtbl[pos]) {
		fprintf(stderr, "%s: macro letter '%c' redefined\n", progname, 'A' + pos);
		return;
	}
	abrvtbl[pos] = p;
	if (3 > (len = strlen(p))) {	/* don't use, but hold letter	*/
		len = 0;
	}
	abrvlen[pos] = len;
	if (0 != verbose) {
		fprintf(stderr, "%s: %c='%s'\n", progname, pos + 'A', p);
	}
}

/* search line for make defines of A-Z Put entries into abrvtbl		(su)
 */
void
srchincl(p)
register char *p;
{
	register char letter, *q, *r;
	register unsigned i;

	if (0 == shortincl || '\000' == *p) {
		return;
	}

	while (isspace(*p))		/* ignore white space */
		++p;
	letter = *p++;
	if (! isupper(letter)) {
		return;
	}

	while (isspace(*p))
		++p;
	if ('=' != *p++) {
		return;
	}

	while (isspace(*p))
		++p;
	i = strlen(p);

	if (NULL == (q = r = malloc(i+1))) {
		OutOfMemory();
	}

	while ('\000' != *p && '#' != *p && ! isspace(*p))
		*q++ = *p++;
	*q = '\000';

	makeabrv(letter-'A', r);
	if (0 != verbose)
		fprintf(stderr, "%s: use macro %c as %s\n", progname, letter, r);
}

/* set up abrev table, spit out the abrevs.			     (ksb/su)
 * Use any A-Z definitions found in Makefile, no duplicates.
 * look at each sting we have noticed as a prefix
 *  (make sure is non-NULL, not too small)
 * macro them
 * output table
 */
void
abrvsetup(fp)
FILE *fp;
{
	register int i;			/* scan tables			*/
	register char *p;		/* surrent abrev. canidate	*/
	register char *q;		/* temp string			*/
	register int slot;		/* slot search point		*/

	srtgti(&abrv);
	while (NULL != (p = srtgets(&abrv))) {
		if (0 != verbose)
			fprintf(stderr, "%s: examine %s, ", progname, p);
		slot = -1;
		for (i = 0; i < MXABR; ++i) {
			q = abrvtbl[i];
			if (NULL == q) {
				if (slot == -1)
					slot = i;
				continue;
			}
			if (0 == strcmp(p, q))
				break;
		}

		/* already in table or no more room in table
		 */
		if (MXABR != i || slot == -1) {
			if (0 != verbose)
				fprintf(stderr, "rejected\n");
			continue;
		}

		/* slot is a known free slot, but we'd rather be mnemonic
		 */
		q = strrchr(p, '/');
		if ((char *)0 != q && isalpha(q[1])) {
			i = q[1] - (islower(q[1]) ? 'a' : 'A');
			if (NULL == abrvtbl[i])
				slot = i;
		}
		if (0 != verbose)
			fprintf(stderr, "accepted as %c\n", slot+'A');
		makeabrv(slot, p);
		fprintf(fp, "%c=%s\n", slot+'A', p);
	}
}

/* find an abbreviation in abrvtbl for string p (if any).		(su)
 * if multiple abbreations work, use longest. (ie: /usr/include &
 * /usr/include/sys; use /usr/include/sys) if found, return index else: MXABR
 */
int
findabr(p)
register char *p;		/* string pointer			*/
{
	register int i;		/* for index				*/
	register int j;		/* found index				*/

	for (i = 0, j = MXABR; i < MXABR; ++i) {
		if (0 == abrvlen[i])
			continue;
		if (0 == strncmp(abrvtbl[i], p, abrvlen[i]))
			if (MXABR == j || abrvlen[i] > abrvlen[j])
				j = i;
	}
	return j;
}
