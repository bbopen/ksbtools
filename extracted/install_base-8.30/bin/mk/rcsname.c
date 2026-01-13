/* $Id: rcsname.c,v 5.2 2009/05/22 16:31:42 ksb Exp $
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include <string.h>

#include "machine.h"

/* Produce an RCS delta file path for the given (existing) file		(ksb)
 */
char *
rcsname(pchFile)
char *pchFile;
{
	static char path[1024];
	register char *p, *q;
	register int fComma;

	q = pchFile;
	strcpy(path, "RCS/");
	strcpy(path+4, q);

	fComma =NULL == (p = strrchr(q, ',')) || 'v' != p[1] || '\000' != p[2];

	if (fComma) {
		strcat(path, ",v");
	}
#if !defined(F_OK)
#define F_OK	0
#endif
	if (0 == access(path+4, F_OK)) {
		return path+4;
	}

	if (0 == access(path, F_OK)) {
		return path;
	}

	if ((char *)0 != (p = strrchr(q, '/'))) {
		p[0] = '\000';
		strcpy(path, q);
		strcat(path, "/RCS/");
		strcat(path, p+1);
		p[0] = '/';

		if (fComma) {
			strcat(path, ",v");
		}
		if (0 == access(path, F_OK)) {
			return path;
		}
	}
	return (char *)0;
}
