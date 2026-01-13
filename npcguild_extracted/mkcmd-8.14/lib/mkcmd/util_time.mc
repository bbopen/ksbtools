/* $Id: util_time.mc,v 8.2 1999/06/12 00:43:58 ksb Exp $
 *	atotm.c - convert ASCII time to seconds since epoch
 *
 *	V. Abell
 *	Purdue University Computing Center
 $Compile(*): ${cc-cc} ${cc_debug--g} -DTEST %f -o %F
 */
/*@Header@*/
/*
 * Copyright 1991 Purdue Research Foundation, West Lafayette, Indiana
 * 47907.  All rights reserved.
 *
 * Written by Victor A. Abell
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. Neither the author nor Purdue University are responsible for any
 *    consequences of the use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Credit to the author and Purdue
 *    University must appear in documentation and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 *! ksb is Kevin S Braunsdorf (ksb@fedex.com)
 *! This version is altered.  ksb changed the name "atotm" to a mkcmd
 *! expression that resolves to "atotm" in the default case.
 */
#ifndef lint
static char copyright[] =
"@(#) Copyright 1991 Purdue Research Foundation.\nAll rights reserved.\n";
#endif

#include <stdio.h>
#include <ctype.h>

#define ISLEAP(y)	(((y) % 4 == 0 && (y) % 100 != 0) || (y) % 400 == 0)



/*
 * Days of the week names
 */
static char *day_names[7] = {
	"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
};


/*
 * Months of the year sizes
 */
static int month_size[12] = {
	31, 28, 31, 30, 31, 30, 31, 31,30, 31, 30, 31
};

static char *month_names[12] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};


/*
 * Time zone abbreviations in alphabetical order for binary search
 *
 * Where there is a choice between duplicates in the following list, the
 * enabled abbreviations have been chosen by selecting current ones over
 * obsolete ones, and by opting for more commonly used ones.
 */
static
struct zone {
	char *nm;		/* zone name */
	int off;		/* UTC offset in minutes */
} zones[] = {
	{ "ACDT",  10*60+30 },	/* Australian Central Daylight */
	{ "ACST",   9*60+30 },	/* Australian Central Standard */
	{ "ADT",      -3*60 },	/* Atlantic Daylight */
	{ "AEDT",     11*60 },	/* Australian Eastern Daylight */
	{ "AES",      10*60 },	/* Australian Eastern Standard */
	{ "AEST",     10*60 },	/* Australian Eastern Standard */
	{ "AHDT",     -9*60 },	/* Alaska-Hawaii Daylight (1967 to 1983) */
	{ "AHST",    -10*60 },	/* Alaska-Hawaii Standard (1967 to 1983) */
	{ "AKDT",     -8*60 },	/* Alaska Daylight */
	{ "AKST",     -9*60 },	/* Alaska Standard */
	{ "AST",      -4*60 },	/* Atlantic Standard */
	{ "AT",       -2*60 },	/* Azores (obsolete) */
	{ "AWDT",      9*60 },	/* Australian Western Daylight */
	{ "AWST",      8*60 },	/* Australian Western Standard */
	{ "BRA",      -3*60 },	/* Brazil */
	{ "BST",       1*60 },	/* British Summer */
/*	{ "BST",      -3*60 },	/* Brazil Standard */
/*	{ "BST",     -11*60 },	/* Bering Summer (1967 to 1983) */
	{ "BT",        3*60 },	/* Baghdad */
/*	{ "BT",      -11*60 },	/* Bering (until 1967) */
	{ "CADT",  10*60+30 },	/* Central Australian Daylight */
	{ "CAST",   9*60+30 },	/* Central Australian Standard */
	{ "CAT",     -10*60 },	/* Central Alaskan Standard (until 1967) */
	{ "CCT",       8*60 },	/* China Coast (obsolete) */
/*	{ "CDT",       9*60 },	/* China Daylight */
	{ "CDT",      -5*60 },	/* Central Daylight */
	{ "CEST",      2*60 },	/* Central European Summer */
	{ "CET",       1*60 },	/* Central European */
	{ "CET DST",   2*60 },	/* Central European Daylight */
	{ "CETDST",    2*60 },	/* Central European Daylight */
/*	{ "CST",       8*60 },	/* China Standard */
/*	{ "CST",    9*60+30 },	/* North/South Australian Central Standard */
	{ "CST",      -6*60 },	/* Central Standard */
	{ "DNT",       1*60 },	/* Dansk Normal */
	{ "DST",       2*60 },	/* Dansk Summer */
	{ "EAST",     10*60 },	/* East Australian Standard */
	{ "EDT",      -4*60 },	/* Eastern Daylight */
	{ "EEST",      3*60 },	/* Eastern European Summer */
	{ "EET",       2*60 },	/* Eastern European */
	{ "EET DST",   3*60 },	/* Eastern European Daylight */
	{ "EETDST",    3*60 },	/* Eastern European Daylight */
	{ "EMT",       1*60 },	/* Norway */
	{ "EST",      -5*60 },	/* Eastern Standard */
/*	{ "EST",      10*60 },	/* Australian Eastern Standard */
	{ "FDT",      -1*60 },	/* Fernando de Noronha Daylight */
	{ "FST",       2*60 },	/* French Summer */
/*	{ "FST",      -2*60 },	/* Fernando de Noronha Standard */
	{ "FWT",       1*60 },	/* French Winter */
	{ "GMT",          0 },	/* Greenwich */
/*	{ "GST",      -3*60 },	/* Greenland Standard (obsolete) */
	{ "GST",      10*60 },	/* Guam Standard */
	{ "HADT",     -9*60 },	/* Hawaiian-Aleutian Daylight */
	{ "HAST",    -10*60 },	/* Hawaiian-Aleutian Standard */
	{ "HDT",   -9*60-30 },	/* Hawaiian Daylight (until 1947) */
	{ "HFE",       2*60 },	/* Heure Francais d'Ete */
	{ "HFH",       1*60 },	/* Heure Francais d'Hiver */
	{ "HKT",       8*60 },	/* Hong Kong */
	{ "HOE",       1*60 },	/* Spain */
	{ "HST",     -10*60 },	/* Hawaiian Standard (-10:30 until 1947) */
	{ "IDLE",     12*60 },	/* International Dateline, East */
	{ "IDLW",    -12*60 },	/* International Dateline, West */
	{ "IDT",       3*60 },	/* Israeli Daylight */
/*	{ "IDT",    4*60+30 },	/* Iran Daylight */
	{ "IST",       2*60 },	/* Israeli Standard */
/*	{ "IST",    3*60+30 },	/* Iran Standard */
/*	{ "IST",    5*60+30 },	/* Indian Standard */
	{ "IT",     3*60+30 },	/* Iran */
	{ "ITA",       1*60 },	/* Italy */
	{ "JST",       9*60 },	/* Japanese Standard */
	{ "JT",     7*60+30 },	/* Java */
	{ "KDT",      10*60 },	/* Korean Daylight */
	{ "KST",       9*60 },	/* Korean Standard */
	{ "LIGT",     10*60 },	/* Melbourne, Australia */
	{ "MAL",       8*60 },	/* Malaysia */
	{ "MAT",       3*60 },	/* Turkey */
	{ "MDT",      -6*60 },	/* Mountain Daylight */
	{ "MEDST",     2*60 },	/* Middle European Daylight */
	{ "MEST",      2*60 },	/* Middle European Summer */
	{ "MESZ",      2*60 },	/* Mittel Europaeische Sommerzeit */
	{ "MET",       1*60 },	/* Middle European */
	{ "MET DST",   2*60 },	/* Middle European Daylight */
	{ "METDST",    2*60 },	/* Middle European Daylight */
	{ "MEWT",      1*60 },	/* Middle European Winter */
	{ "MEX",      -6*60 },	/* Mexico */
	{ "MEZ",       1*60 },	/* Mittel Europaeische Zeit */
	{ "MSD",       3*60 },	/* Moscow Summer */
	{ "MSK",       2*60 },	/* Moscow Winter */
	{ "MST",      -7*60 },	/* Mountain Standard */
	{ "MT",     8*60+30 },	/* Moluccas (obsolete) */
	{ "NDT",   -2*60-30 },	/* Newfoundland Daylight */
	{ "NFT",   -3*60-30 },	/* Newfoundland */
	{ "NOR",       1*60 },	/* Norway */
/*	{ "NST",    8*60+30 },	/* North Sumatran (obsolete) */
	{ "NST",   -3*60-30 },	/* Newfoundland Standard */
	{ "NT",      -11*60 },	/* Nome (until 1967) */
	{ "NZDT",     13*60 },	/* New Zealand Daylight */
	{ "NZST",     12*60 },	/* New Zealand Standard */
	{ "NZT",      12*60 },	/* New Zealand */
	{ "PDT",      -7*60 },	/* Pacific Daylight */
	{ "PST",      -8*60 },	/* Pacific Standard */
	{ "SADT",  10*60+30 },	/* South Australian Daylight */
	{ "SAST",   9*60+30 },	/* South Australian Standard */
	{ "SET",       1*60 },	/* European -- Prague, Vienna */
	{ "SST",       2*60 },	/* Swedish Summer */
/*	{ "SST",       7*60 },	/* South Sumatran (obsolete) */
/*	{ "SST",       8*60 },	/* Singapore Standard */
/*	{ "SST",     -11*60 },	/* Samoa Standard */
	{ "SWT",       1*60 },	/* Swedish */
	{ "THA",       7*60 },	/* Thailand Standard */
	{ "TST",       3*60 },	/* Turkish Standard */
	{ "UT",           0 },	/* Universal */
	{ "UTC",          0 },	/* Universal Time Coordinated */
	{ "UTZ",      -3*60 },	/* Western Greenland Standard */
	{ "VTZ",      -2*60 },	/* Eastern Greenland Standard */
/*	{ "VTZ",      -2*60 },	/* Western Greenland Daylight */
	{ "WADT",      9*60 },	/* Western Australian Daylight */
	{ "WAST",      8*60 },	/* Western Australian Standard */
	{ "WAT",       1*60 },	/* West African (obsolete) */
	{ "WDT",       9*60 },	/* Western Australian Daylight */
	{ "WEST",      1*60 },	/* Western European Summer */
	{ "WET",          0 },	/* Western European */
	{ "WET DST",   1*60 },	/* Western European Daylight */
	{ "WETDST",    1*60 },	/* Western European Daylight */
	{ "WST",       8*60 },	/* Western Australian Standard */
	{ "WTZ",      -1*60 },	/* Eastern Greenland Daylight */
	{ "WUT",       1*60 },	/* Austria */
	{ "YDT",      -8*60 },	/* Yukon Daylight */
	{ "YST",      -9*60 } 	/* Yukon Standard */
};


/*
 * adjyear(y) - adjust year						(abe)
 *
 * entry:
 *	y = year to be adjusted
 *
 * exit:
 *	return = adjusted year
 *		 -1 = error detected
 */
static int
adjyear(y)
	int y;			/* year to be adjusted */
{
	if (y  < 100) {

	/*
	 * This test will work until 2069.  I'll be 131 then and
	 * probably won't care.  :-)
	 */
		if (y >= 70)
			y += 1900;
		else
			y += 2000;
	} else if (y < 1970)
		return(-1);
	return(y);
}


/*
 * arealpha(a, n) - are the characters alphabetic?			(abe)
 *
 * entry:
 *	a = pointer to characters
 *	n = number of characters to be tested
 *
 * exit:
 *	return = 0 if not all characters are alphabetic
 *		 1 if all characters are alphabetic
 */
static int
arealpha(a, n)
	char *a;		/* pointer to characters */
	int n;			/* number of characters to be tested */
{

	while (n > 0) {
		if(*a == '\0' || !isascii(*a) || !isalpha(*a))
			return(0);
		a++;
		n--;
	}
	return(1);
}


/*
 * cincmp(s1, s2, n) - case insensitive string compare by length	(abe)
 *
 * entry:
 *	s1 = pointer to first string
 *	s2 = pointer to second string
 *	n  = number of characters to compare
 *
 * exit:
 *	return = 1 if strings match
 *	       = 0 if strings do not match
 */
static int
cincmp(s1, s2, n)
	char *s1, *s2;		/* pointers to strings */
	int n;			/* number of characters to compare */
{
	for (; n; n--, s1++, s2++) {
		if (*s1 == *s2) {
			if (*s1 == '\0')
				return(1);
			continue;
		}
		if (*s1 == '\0' || *s2 == '\0')
			return(0);
		if (isascii(*s1) && isascii(*s2)) {
			if ((isupper(*s1) ? tolower(*s1) : *s1)
			==  (isupper(*s2) ? tolower(*s2) : *s2))
				continue;
		}
		return(0);
	}
	return(1);
}


/*
 * convday(dcp, dp) - convert day					(abe)
 *
 * entry:
 *	dcp = day character pointer
 *	dp  = day result pointer
 *
 * exit:
 *	return = number of characters processed
 *		 -1 = error detected
 */
static int
convday(dcp, dp)
	char *dcp;		/* day character pointer */
	int *dp;		/* day result pointer */
{
	char *cp;
	int d;

	cp = dcp;
	for (d = 0; *cp && isascii(*cp) && isdigit(*cp); cp++)
		d = (d * 10) + (*cp - '0');
	if (d > 0) {

	/*
	 * Check for legal terminator - space, slash, comma, dash, period,
	 * "nd", "rd", "st",  or "th"
	 */
		switch (*cp) {

		case '\0':
			break;
		case ' ':
		case '/':
		case ',':
		case '-':
		case '.':
			cp++;
			break;
		case 'n':
		case 'N':
		case 'r':
		case 'R':
			cp++;
			if (*cp == 'd' || *cp == 'D') {
				cp++;
				break;
			}
			return(-1);
		case 's':
		case 'S':
			cp++;
			if (*cp == 't' || *cp == 'T') {
				cp++;
				break;
			}
			return(-1);
		case 't':
		case 'T':
			cp++;
			if (*cp == 'h' || *cp == 'H') {
				cp++;
				break;
			}
			/* fall through */
		default:
			return(-1);
		}
	}
	while (*cp && isascii(*cp) && isspace(*cp))
		cp++;
	*dp = d;
	return(cp - dcp);
}


/*
 * convmonth(mcp, mp) - convert month					(abe)
 *
 * entry:
 *	mcp = month character pointer
 *	mp  = month result pointer
 *
 * exit:
 *	return = number of characters processed
 *		 -1 = error detected
 */
static int
convmonth(mcp, mp)
	char *mcp;		/* month character pointer */
	int *mp;		/* month result pointer */
{
	char *cp;
	int i;

	if ( ! arealpha(mcp, 3))
		return(-1);

	/*
	 * Look up the month name.
	 */
	for (i = 0; i < 12; i++) {
		if (cincmp(mcp, month_names[i], 3))
			break;
	}
	if (i > 11)
		return(-1);
	for (cp = mcp; *cp && isascii(*cp) && isalpha(*cp); cp++)
		;
	while (*cp && isascii(*cp) && (isspace(*cp) || ispunct(*cp)))
		cp++;
	*mp = i;
	return(cp - mcp);
}


/*
 * convsdate(dcp, dp, mp, yp) - convert slashed date			(abe)
 *
 * The form is deduced by the following rules:
 *
 *	xx/yy/zz ->	yy/mm/dd	if xx > 31, 0 < yy < 13, 0 < zz < 32
 *	xx/yy/zz ->	mm/dd/yy	if zz > 31, 0 < xx < 13, 0 < yy < 32
 *	xx/yy/zz ->	dd/mm/yy	if zz > 31, 0 < xx < 32, 0 < yy < 13
 *
 * These rules are imperfect -- so is the form.  :-)
 *
 * entry:
 *	dcp = date character pointer
 *	dp  = day result pointer
 *	mp  = month result pointer
 *	yp  = year result pointer
 *
 * exit:
 *	return = number of characters processed
 *		 -1 = error detected
 */
static int
convsdate(dcp, dp, mp, yp)
	char *dcp;		/* date character pointer */
	int *dp, *mp, *yp;	/* result pointers */
{
	char *cp;
	int j, r[3], y;

	for (cp = dcp, j = 0; j < 3; j++) {
		for (r[j] = 0; *cp && isascii(*cp) && isdigit(*cp); cp++)
			r[j] = (10 * r[j]) + *cp - '0';
		switch (j) {
		case 0:
		case 1:
			if (*cp == '/')
				break;
			return(-1);
		case 2:
			if (*cp == '\0')
				break;
			if (isascii(*cp) && (isspace(*cp) || ispunct(*cp)))
				break;
			return(-1);
		}
		cp++;
	}
	if (r[0] > 31 && r[1] > 0 && r[1] < 13 && r[2] > 0 && r[2] < 32) {

	/*
	 * ANSI form - yy/mm/dd
	 */
		y = r[0];
		*mp = r[1] - 1;
		*dp = r[2];
	} else if (r[2] > 31 && r[0] > 0 && r[0] < 13 && r[1] > 0 && r[1] < 32){

	/*
	 * U.S. - mm/dd/yyy
	 */
		y = r[2];
		*mp = r[0] - 1;
		*dp = r[1];
	} else if (r[2] > 31 && r[0] > 0 && r[0] < 32 && r[1] > 0 && r[1] < 13){

	/*
	 * European - dd/mm/yy
	 */
		y = r[2];
		*mp = r[1] - 1;
		*dp = r[0];
	} else
		return(-1);
	if ((*yp = adjyear(y)) < 0)
		return(-1);
	while (*cp && isascii(*cp) && isspace(*cp))
		cp++;
	return(cp - dcp);
}


/*
 * convtime(tcp, c, hp, mp, sp) - convert time				(abe)
 *
 * entry:
 *	tcp = time character pointer
 *	c   = 1 if colon separation is required
 *	hp  = hour result pointer
 *	mp  = minutes result pointer
 *	sp  = seconds result pointer
 *
 * exit:
 *	return = number of characters processed
 *		 -1 = error detected
 */
static int
convtime(tcp, c, hp, mp, sp)
	char *tcp;		/* time character pointer */
	int c;			/* colon requirement */
	int *hp, *mp, *sp;	/* result pointers */
{
	char *cp, *t;
	int r;

	cp = tcp;
	if (*cp && isascii(*cp) && isdigit(*cp)) {

	/*
	 * Assemble hours and look for hhmm form when the caller
	 * doesn't require colon separators.
	 */
		for (r = 0; *cp && isascii(*cp) && isdigit(*cp); cp++)
			r = (r * 10) + (*cp - '0');
		if (r < 0 || r > 23 || *cp++ != ':') {
			if ((cp - tcp) == 4 && c == 0) {
				*hp = r / 100;
				*mp = r % 100;
				*sp = 0;
				goto time_exit;
			}
			return(-1);
		}
		*hp = r;
	/*
	 * Assemble minutes.
	 */
		for (r = 0; *cp && isascii(*cp) && isdigit(*cp); cp++)
			r = (r * 10) + (*cp - '0');
		if (r < 0 || r > 59)
			return(-1);
		*mp = r;
	/*
	 * Assemble seconds (optional).
	 */
		r = 0;
		if (*cp == ':') {
			for (cp++; *cp && isascii(*cp) && isdigit(*cp); cp++)
				r = (r * 10) + (*cp - '0');
			if (r < 0 || r > 59)
				return(-1);
		}
		*sp = r;
	/*
	 * Skip trailing fraction and process AM/PM forms.
	 */

time_exit:
		if (*cp == '.') {
			cp++;
			while (*cp && isascii(*cp) && isdigit(*cp))
				cp++;
		}
		while (*cp && isascii(*cp) && isspace(*cp))
			cp++;
		r = 0;
		switch (*cp) {

		case 'p':
		case 'P':
			r = 12;
			/* fall through */

		case 'a':
		case 'A':
			t = cp + 1;
			if (*t == '.')
				t++;
			if (*t != 'm' && *t != 'M')
				break;
			t++;
			if (*t == '\0'
			|| (isascii(*t) && (isspace(*t) || ispunct(*t)))) {
				if((*hp += r) > 23)
					return(-1);
				cp = (*t) ? t + 1 : t;
			}
		}
		while (*cp && isascii(*cp) && isspace(*cp))
			cp++;
		return(cp - tcp);
	} else {
/*
 * If there's no leading digit, there's probably no time.
 */
		*hp = *mp = *sp = 0;
		return(0);
	}
}


/*
 * convyear(ycp, yip) - convert year					(abe)
 *
 * entry:
 *	ycp = pointer to year characters
 *	yip = year result pointer
 *
 * exit:
 *	return = number of characters processed
 *		 -1 = error detected
 */
static int
convyear(ycp, yip)
	char *ycp;		/* pointer to year characters */
	int *yip;		/* year result pointer */
{
	char *cp;
	int yr;

	for (cp = ycp, yr = 0; *cp && isascii(*cp) && isdigit(*cp); cp++)
		yr = (yr * 10) + (*cp - '0');
	switch (*cp) {

	case '\0':
		break;
	case ' ':
	case ',':
	case '.':
		cp++;
		break;
	default:
		return(-1);
	}
	if ((yr = adjyear(yr)) < 0)
		return(-1);
	while (*cp && isascii(*cp) && isspace(*cp))
		cp++;
	*yip = yr;
	return(cp - ycp);
}


/*
 * zonelen(z) - compute length of zone string				(abe)
 *
 * entry:
 *	z = pointer to start of zone information
 *
 * exit:
 *	return = zone string length
 */
static int
zonelen(z)
	char *z;		/* pointer to start of zone information */
{
	int len;

	for (len = 0; *z; len++, z++) {
		if ( ! isascii(*z))
			break;
		if (isalpha(*z))
			continue;
		if ( ! isdigit(*z))
			break;
		if (len == 0)
			break;
	}
	if (*z == '\0' || *z != ' ')
		return(len);
	z++;
	if (cincmp(z, "DST", 3))
		len += 4;
	return(len);
}


/*
 * convzone(zcp, mp) - convert zone					(abe)
 *
 * entry:
 *	zcp = zone character pointer
 *	mp  = minute result pointer
 *
 * exit:
 *	return = number of characters processed
 *		 -1 = error detected
 */
static int
convzone(zcp, mp)
	char *zcp;		/* zone character pointer */
	int *mp;		/* minute result pointer */
{
	char *cp, *tp, zbuf[8];
	int hi, i, j, k, low, mid;

	cp = zcp;
	if (*cp && isascii(*cp)) {
		if (*cp == '-' && *(cp+1) && isascii(*(cp+1))
		&& isalpha(*(cp+1)))
			cp++;
		if (isalpha(*cp)) {

		/*
		 * Look up time zone abbreviation.
		 */
			if ((j = zonelen(cp)) < 2 || j > 7)
				return(-1);
			for (i = 0, tp = cp; i < j; i++, tp++)
				zbuf[i] = islower(*tp) ? toupper(*tp) : *tp;
			zbuf[j] = '\0';
			low = mid = 0;
			hi = (sizeof(zones) / sizeof(struct zone)) - 1;
			while (low <= hi) {
				mid = (low + hi) / 2;
				if (zones[mid].nm == NULL)
					exit(1);
				if ((i = strcmp(zbuf, zones[mid].nm)) == 0) {
					*mp -= zones[mid].off;
					cp += j;
					break;
				}
				if (i < 0)
					hi = mid - 1;
				else
					low = mid + 1;
			}
			if (low > hi)
				return(-1);
		}
		if (*cp == '-' || *cp == '+') {

		/*
		 * Convert offset: [+-]nnnn
		 *	       or
		 *		   [+-]nn:nn
		 */
			i = (*cp++ == '-') ? 1 : -1;
			for (j = 0; *cp && isascii(*cp) && isdigit(*cp); cp++)
				j = (j * 10) + (*cp - '0');
			if (j > 1159)
				return(-1);
			k = 0;
			if (*cp == ':' && j < 12) {
				cp++;
				while (*cp && isascii(*cp) && isdigit(*cp))
					k = (k * 10) + (*cp++ - '0');
				if (k > 59)
					return(-1);
			} else {
				if (j > 100) {
					k = j % 100;
					j = j / 100;
				} else if (j > 11)
					return(-1);
			}
			*mp += i * ((j * 60) + k);
		}
		while (*cp && isascii(*cp) && isspace(*cp))
			cp++;
		return(cp - zcp);
	}
	return(-1);
}


/*
 * skipday(dcp) - skip day name						(abe)
 *
 * entry:
 *	dcp = day name character pointer
 *
 * exit:
 *	return = number of characters skipped
 *		 -1 = no day name skipped
 */
static int
skipday(dcp)
	char *dcp;		/* day name character pointer */
{
	char *cp;
	int i;

	if ( ! arealpha(dcp, 3))
		return(-1);
	for (cp = dcp, i = 0; i < 7; i++) {
		if (cincmp(cp, day_names[i], 3))
			break;
	}
	if (i > 6)
		return(-1);
	while (*cp && isascii(*cp) && (isalpha(*cp) || ispunct(*cp)))
			cp++;
	while (*cp && isascii(*cp) && isspace(*cp))
		cp++;
	return(cp - dcp);
}


/*
 * atotm(a, tz) - convert an ASCII time to the number of seconds since	(abe)
 *		  the epoch
 *
 * Examples of ASCII time forms that this function can convert include
 * the following:
 *
 *	Mon May 13 10:49:00 1991		(ctime(3) output)
 *	Mon May 13 10:49:00 1991 YST
 *	Mon., May 13, 10:49:00 TST 1991
 *	Wed, 24 April 91 12:28:30 -0500
 *	Mon,  6 May 91 17:15:37 +5
 *	Thu,  28 Mar 91 13:20 +02:00
 *	Saturday, 2 Mar 91 01:38:58 pst
 *	Tue 21 Apr 87 15:37:41-EDT
 *	Mon, 01 Apr 91, 15:15:15 MET
 *	Wed, 1991 Apr 3   11:22 EST
 *	Thursday, Mar 28, 1988
 *	Friday, March 29,1975
 *	8 May, 1990
 *	August, 1990.
 *	23rd April, 1991
 *	22nd June, 1978
 *	30th October 1990
 *	7 May 91 16:29:08 GMT
 *	10 July 90 10:15:00 GMT+0800
 *	2-MAY-1984 17:57:19
 *	17/Dec/90 06:18:01 AST
 *	12/25/75 01:30:00 -0500			(mm/dd/yy -- U.S.)
 *	Thu 75/12/25 10:30 EDT			(yy/mm/dd -- ANSI)
 *	25/12/75				(mm/dd/yy - European)
 *	04/25/90 Wed 18:28:44
 *
 * See zones[] for the names of the time zone abbreviations that this
 * function recognizes.  Zones that cannot be interpreted, including
 * excessively large numeric offsets, are silently ignored.
 *
 * The default time is 00:00:00 in the default zone, supplied in the tz
 * argument.  The default day is 1.
 *
 * Conversion of xx/yy/zz date forms is partly guesswork.  (See the
 * comments accompanying the convsdate() function.)
 *
 * entry:
 *	a  = pointer to ASCII time
 *	tz = default zone -- the GMT/UTC offset in minutes for times
 *	     without a zone specification:
 *	         A negative number represents zones west from GMT/UTC
 *		     to the international dateline (e. g., -5*60 for EST).
 *	         A positive number represents zones east from GMT/UTC
 *		     to the international dateline (e. g., 1*60 for MET).
 *
 * exit:
 *	return = number of seconds since epoch in local time, suitable for
 *		 input to ctime(3)
 *	       = -1L if ASCII time cannot be converted
 */
long
%%K<Atotm>1ev(a, tz)
	char *a;		/* pointer to ASCII date */
	int tz;			/* time zone offset */
{
	int day, hours, minutes, month, seconds, year, zone;
	register int i;
	long tm;

	if ((char *)0 == a)
		return(-1L);
	day = month = year = zone = -1;
	while (*a && isascii(*a) && isspace(*a))
		a++;
	if ( ! *a)
		return(-1L);
/*
 * Skip leading day of the week name, or process leading month of the
 * year name.
 */
	if (*a && isascii(*a) && isalpha(*a)) {
		if ( ! arealpha(a, 3))
			return(-1L);
		if ((i = skipday(a)) > 0) {
			a += i;
		} else {
			if ((i = convmonth(a, &month)) < 0)
				return(-1L);
			a += i;
		}
	}
/*
 * Convert: MMM dd
 *	or
 *	    mm/dd/yy [day name]
 *	    yy/mm/dd [day name]
 *	    dd/mm/yy [day name]
 *	or
 *	    dd
 *	or
 *	    yyyy MMM
 */
	if (month == -1 && *a && isascii(*a) && isalpha(*a)) {

	/*
	 * Leading alpha implies month name.
	 */
		if ((i = convmonth(a, &month)) < 0)
			return(-1L);
		a += i;
	}
	if (month == -1 && *a) {

	/*
	 * Check for possible mm/dd/yy [day_name] form.
	 */
		if ((i = convsdate(a, &day, &month, &year)) > 0) {
			a += i;
			if ((i = skipday(a)) > 0)
				a += i;
		}
	}
	if (day == -1 && *a) {

	/*
	 * Convert simple day number.
	 */
		if ((i = convday(a, &day)) < 0)
			return(-1L);
		a += i;
		if (day > 31 && month == -1 && *a && isascii(*a)
		&& isalpha(*a)) {

		/*
		 * If the assembled day number is too large and no month has
		 * been assembled and the next character is an alphabetic,
		 * assume the form "yyyy MMM".
		 */
			if ((year = adjyear(day)) < 0)
				return(-1);
			day = -1;
		}
		if (day > 31 && (*a == '.' || *a == '\0')) {
		/*
		 * If the assembled day number is too large and the month
		 * has been assembled and the next character ends the
		 * time string, assume the day number is the year and the
		 * day is one.
		 */
			if ((year = adjyear(day)) < 0)
				return(-1);
			day = 1;
		}
	}
/*
 * Convert: MMM
 *	or
 *	    MMM dd
 */
	if (month == -1 && *a) {
		if ((i = convmonth(a, &month)) < 0)
			return(-1L);
		a += i;
	}
	if (day == -1 && *a) {
		if ((i = convday(a, &day)) < 0)
			return(-1L);
		a += i;
	}
/*
 * Convert: yyyy [hh:mm[:ss]][ zzz]
 *	or
 *	    [[h]h:mm[:ss]][ zzz][ yyyy]
 *
 * The time is optional, and time seconds are optional.  If there is
 * no time, 00:00:00 is the default.
 */
	if (*a && isascii(*a) && isdigit(*a) && (*(a+1) == ':' ||
	    (isascii(*(a+1)) && isdigit(*(a+1)) && *(a+2) == ':'))) {

	/*
	 * Two digits, followed by a colon, indicate a leading time.
	 */
		if ((i = convtime(a, 1, &hours, &minutes, &seconds)) < 0)
			return(-1L);
		a += i;
		if (*a && zone == -1) {
			if ((i = convzone(a, &minutes)) > 0) {
				a += i;
				zone = 1;
			}
		}
		if (*a) {
			if (year != -1 || (i = convyear(a, &year)) < 0)
				return(-1L);
			a += i;
		} else if (year == -1)
			return(-1L);
	} else {

	/*
	 * More than two leading digits or no colon after two, indicates
	 * a leading year.
	 */
		if (*a) {
			if (year != -1 || (i = convyear(a, &year)) < 0)
				return(-1L);
			a += i;
		}
		if (*a) {
			if ((i = convtime(a, 0, &hours, &minutes, &seconds))
			> 0)
				a += i;
			else
				hours = minutes = seconds = 0;
		} else
			hours = minutes = seconds = 0;
	}
/*
 * If no time zone has been detected, try for one.
 */
	if (*a && zone == -1) {
		if (convzone(a, &minutes) > 0) {
			a+= i;
			zone = 1;
		}
	}
	if (zone == -1)
		minutes -= tz;
/*
 * Adjust for February in leap/non-leap years, then check for a valid day.
 */
	if (ISLEAP(year))
		month_size[1] = 29;
	else
		month_size[1] = 28;
	if (day < 0 || month < 0 || day > month_size[month] || year < 0)
		return(-1L);
/*
 * Calculate seconds since the epoch.
 */
	for (i = 1970, tm = 0L; i < year; i++)
		tm += (ISLEAP(i) ? 366 : 365);
	while (--month >= 0)
		tm += month_size[month];
	tm += (long) day - 1l;
	tm = (tm * 24l) + (long) hours;
	tm = (tm * 60l) + (long) minutes;
	tm = (tm * 60l) + (long) seconds;
	return(tm);
}

/*@Remove@*/
#if defined(TEST)

/*@Explode main@*/
char *progname =
	"$Id ksb $";

#include <sys/types.h>
#include <sting.h>
#include <time.h>

/* test driver								(ksb)
 */
int
main(argc, argv)
int argc;
char **argv;
{
	auto time_t tGot;

	if ((char *)0 == (progname = strrchr(argv[0], '/'))) {
		progname = argv[0];
	} else {
		++progname;
	}
	switch (argc) {
	case 2:
		tGot = atotm(argv[1], 0);
		printf("%s: %s (%ld)\n", progname, argv[1], tGot);
		printf("%s", ctime(& tGot));
		break;
	default:
		fprintf(stderr, "%s: usage time\n", progname);
		exit(1);
	}
	exit(0);
}
/*@Remove@*/
#endif
