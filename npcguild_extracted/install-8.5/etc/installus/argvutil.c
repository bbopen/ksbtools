/* $Id: argvutil.c,v 2.1 1997/10/15 20:38:29 ksb Exp $
 *
 * routines for playing with argv's
 *
 * Ben Jackson (ben@ben.com)
 */

#include <ctype.h>

/*
 * Append one argv to another
 */
void
ArgvAppend(pargc, pargv, argc, argv)
int *pargc;
char ***pargv;
int argc;
char **argv;
{
	*pargv = (char **) realloc(*pargv, (argc + *pargc + 1) * sizeof(char *));
	while (argc--)
		(*pargv)[(*pargc)++] = *(argv++);
	(*pargv)[*pargc] = (char *)0;
}

/*
 * Take a whitespace-separated string and append it to an argv.      (bj)
 */
void
Argvize(pargc, pargv, pcArgs)
int *pargc;
char ***pargv;
char *pcArgs;
{
	register char *pcCur, *pcTemp;

	if ((char *)0 == pcArgs || '\000' == pcArgs[0]) {
		/* that was easy! */
		return;
	}

	/* clone the string so we can stomp \0's into it and use it
	 * in the argv
	 */
	pcCur = (char *) malloc(strlen(pcArgs) + 1);
	strcpy(pcCur, pcArgs);

	for (;;) {
		/* skip whitespace
		 */
		while ('\000' != pcCur[0] && isspace(pcCur[0]))
			++pcCur;
		if ('\000' == pcCur[0])
			break;

		/* remember where it starts, find the end
		 */
		pcTemp = pcCur;
		while ('\000' != pcCur[0] && !isspace(pcCur[0]))
			++pcCur;
		if ('\000' != pcCur[0])
			*(pcCur++) = '\000';

		*pargv = (char **) realloc(*pargv, (1 + ++(*pargc)) * sizeof(char *));
		(*pargv)[*pargc - 1] = pcTemp;
		(*pargv)[*pargc] = (char *)0;
	}
}
