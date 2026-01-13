/* $Id: breakargs.c,v 3.2 2003/04/20 19:36:49 ksb Exp $
 * breakargs - break a string into a string vector for execv.		(ksb)
 * put in a fix for cmds lines with "string string" in them 
 * Mon Aug 25 13:34:27 EST 1986 (ksb) 
 *
 * $Compile: ${cc-cc} ${cc_debug--g} -DTEST -o %F %f
 */
#include <sys/types.h>
#include <stdio.h>		/* for nothing, really			*/
#include <errno.h>

#include "machine.h"
#include "main.h"

#define SPC '\040'		/* ascii space				*/

/*
 * match quotes in an argument
 */
static char *mynext(pc)
register char *pc;
{
	register int fQuote;

	for (fQuote = 0; '\000' != *pc && ((SPC != *pc && '\t' != *pc) || fQuote); ++pc) {
#if 0
		if ('\\' == *pc) {
			continue;
		}
#endif /* we don't wanna do backslash here too */
		switch (fQuote) {
		default:
		case 0:
			if ('"' == *pc) {
				fQuote = 1;
			} else if ('\'' == *pc) {
				fQuote = 2;
			}
			break;
		case 1:
			if ('"' == *pc)
				fQuote = 0;
			break;
		case 2:
			if ('\'' == *pc)
				fQuote = 0;
			break;
		}
	}
	switch (fQuote) {
	case 0:
		return pc;
		break;
	case 1:
		(void)fprintf(stderr, "%s: unmatched \"\n", progname);
		return NULL;
	case 2:
		(void)fprintf(stderr, "%s: unmatched '\n", progname);
		return NULL;
	default:
		/* internal error */
		abort();
		break;
	}
	/* NOTREACHED */
}


/*
 * passed a string to break up into argv[]
 * allocates enough core to do it (which should be free'd) and returns
 * an argv pointer
 */
char **breakargs(cmd)
char *cmd;
{
	register char *pc;		/* tmp				 */
	register char **v;		/* vector of commands returned	 */
	register unsigned int uiSum;	/* bytes for malloc		 */
	register int i;			/* number of args		 */
	register char *pcSave;		/* save old position		 */

	pc = cmd;
	while (SPC == *pc || '\t' == *pc)
		pc++;
	cmd = pc;		/* no leading spaces		 */
	uiSum = sizeof(char *);
	i = 0;
	while ('\000' != *pc) {	/* space for argv[];		 */
		++i;
		pcSave = pc;
		pc = mynext(pc);
		if (NULL == pc) {
			return (char **)0;
		}
		uiSum += sizeof(char *) + 1 + (unsigned) (pc - pcSave);
		while (SPC == *pc || '\t' == *pc)
			pc++;
	}
	++i;
	/* vector starts at v, copy of string follows NULL pointer
	 */
	v = (char **)malloc(uiSum + 1);
	if ((char **)0 == v) {
		errno = ENOMEM;
		return (char **)0;
	}
	pc = (char *) v + i * sizeof(char *);
	i = 0;
	while ('\000' != *cmd) {
		v[i++] = pc;
		pcSave = cmd;
		cmd = mynext(cmd);
		if (NULL == cmd)
			return (char **)0;
		if ('\000' != *cmd)
			*cmd++ = '\000';
		(void)strcpy(pc, pcSave);
		pc += strlen(pc);
		++pc;
		while (SPC == *cmd || '\t' == *cmd)
			++cmd;
	}
	v[i] = (char *) NULL;

	return v;
}


#if defined TEST
char *progname = "foo";
int
main()
{
	static char buf[1024];
	char **ppc;

	ppc = breakargs("\"this is a test\"");
	while ((char *)0 != *ppc) {
		(void)fprintf(stdout, "/%s/\n", *ppc);
		++ppc;
	}
	exit(0);
}
#endif
