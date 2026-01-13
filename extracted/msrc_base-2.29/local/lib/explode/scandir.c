/*@Version $Revision: 6.12 $@*/
/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * This version has been updated by ksb, largely to make it more "ANSI C".
 * $Id: scandir.c,v 6.12 2009/10/16 15:48:33 ksb Exp $
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)scandir.c	5.3 (Berkeley) 6/18/88";
#endif /* LIBC_SCCS and not lint */

/*
 * Scan the directory dirname calling select to make a list of selected
 * directory entries then sort using qsort and compare routine dcomp.
 * Returns the number of entries and a pointer to a list of pointers to
 * struct direct (through namelist). Returns -1 if there were any errors.
 */

#include <sys/types.h>
#include <sys/stat.h>
/*& #include <sys/dir.h> &*/
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int
scandir(const char *dirname,
	struct dirent *(*namelist[]),
	int (*mtselect)(const struct dirent *),
	int (*dcomp)(const struct dirent **, const struct dirent **))
{
	register struct dirent *d, *p, **names;
	register int nitems;
	register char *cp1, *cp2;
	auto struct stat stb;
	long arraysz;
	DIR *dirp;

	if ((dirp = opendir(dirname)) == NULL)
		return -1;
	if (fstat(dirp->dd_fd, &stb) < 0)
		return -1;

	/* Estimate the array size by taking the size of the directory file
	 * and dividing it by a multiple of the minimum size entry.
	 */
	arraysz = 1+((stb.st_size / 24) | 7);
	names = (struct dirent **)malloc(arraysz * sizeof(struct dirent *));
	if (names == NULL)
		return -1;

	nitems = 0;
	while ((d = readdir(dirp)) != NULL) {
		/* compensate for linux /sys/module/usbcore/parameters bug:
		 * we get an emtpy node name which is a directory.
		 */
		if ('\000' == d->d_name[0] || (mtselect != NULL && !(*mtselect)(d)))
			continue;	/* just selected names */
		/*
		 * Make a minimum size copy of the data
		 */
		p = (struct dirent *)malloc((d->d_reclen|3) + 1);
		if (p == NULL)
			return -1;
		p->d_ino = d->d_ino;
		p->d_reclen = d->d_reclen;
		/* p->d_namlen = d->d_namlen; */
		for (cp1 = p->d_name, cp2 = d->d_name; '\000' != (*cp1++ = *cp2++); );
		/* Check to make sure the array has space left or
		 * realloc the maximum size.
		 */
		if (++nitems >= arraysz) {
			arraysz  += 32;
			names = (struct dirent **)realloc((char *)names,
				arraysz * sizeof(struct dirent *));
			if (names == NULL)
				return -1;
		}
		names[nitems-1] = p;
	}
	closedir(dirp);
	if (nitems && dcomp != NULL)
		qsort(names, nitems, sizeof(struct dirent *), (int (*)(const void *, const void *))dcomp);
	*namelist = names;
	return nitems;
}

/* Alphabetic order comparison routine for those who want it.
 */
int
alphasort(const struct dirent **d1, const struct dirent **d2)
{
	return strcmp((*d1)->d_name, (*d2)->d_name);
}
