/* $Id: listtombs.c,v 3.5 2003/04/20 19:36:49 ksb Exp $
 * do the tomb command (each command has a file)		(becker)
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>

#include "machine.h"
#include "main.h"
#include "lists.h"
#include "init.h"

/*
 * date2str - convert the given time to the way we want to print it
 */
char *
date2str(time)
time_t time;
{
	extern char *strrchr();
	register char *pcEoln, *pc;

	pc = asctime(localtime(& time));
	if (NULL != (pcEoln = strrchr(pc, '\n'))) {
		*pcEoln = '\000';
	}

	return pc;
}


/* show a header for the function below
 */
void
ListHeader()
{
	(void)fprintf(stdout, "%24s  %-38s  %12s\n", "date entombed", "file name", "file size");
	(void)fprintf(stdout, "%24s  %-38s  %12s\n", "---- --------", "---- ----", "---- ----");
}

/* print a listing of the files that have been entombed for the user,	(ksb)
 * which means printing the contents of the linked list we built earlier.
 */
int
LsEntombed(pLE)
LE *pLE;
{
	(void)fprintf(stdout, "%24s  %-38s  %12d\n", date2str(pLE->date), pLE->name, pLE->size);
	return 0;
}
