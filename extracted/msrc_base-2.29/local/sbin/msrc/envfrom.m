#!mkcmd
# $Id: envfrom.m,v 1.4 2008/11/07 22:48:53 ksb Exp $
# Code to read files in msrc $ENV format into memory.
from '<stdio.h>'
from '<ctype.h>'
from '<stdlib.h>'
from '<string.h>'
from '<sysexits.h>'

require "util_ppm.m" "util_errno.m"

%hi
extern char *EnvFromFile(char **ppcList, unsigned uCount);
%%

%c
/* Cat all the files together into a malloc'd blob of text		(ksb)
 * We remove comment lines (for #! and mk usage) and puts missing
 * white-space between files.  Newlines become spaces, BTW.
 * We skip the file named "." so you can force "no file" into the
 * list.
 *
 * Added $ENV on a line by itself to interpolate the variable. This
 * allows $HXMD to include the (preset) value in the $HXMD_PASS variable,
 * if the source wants to allow it.
 */
char *
EnvFromFile(char **ppcList, unsigned uCount)
{
	register int c, fNeedPad, fComment, fCol1;
	register FILE *fpIn;
	register char *pcRet;
	register unsigned uEnd, uEnv;
	auto PPM_BUF PPMEver;

	pcRet = (char *)0;
	util_ppm_init(& PPMEver, sizeof(char *), 2048);
	uEnd = 0;
	for (fComment = fNeedPad = 0; uCount > 0; --uCount, ++ppcList) {
		if ('.' == ppcList[0][0] && '\000' == ppcList[0][1]) {
			continue;
		}
		if ((FILE *)0 == (fpIn = fopen(*ppcList, "r"))) {
			fprintf(stderr, "%s: fopen: %s: %s\n", progname, *ppcList, strerror(errno));
			exit(EX_NOINPUT);
		}
		pcRet = util_ppm_size(& PPMEver, (uEnd|1023)+1);
		if (fNeedPad) {
			pcRet[uEnd++] = ' ';
		}
		fCol1 = 1, uEnv = 0;
		while (EOF != (c = getc(fpIn))) {
			if (fComment) {
				if ('\n' == c) {
					fNeedPad = fComment = 0;
					fCol1 = 1;
				}
				continue;
			}
			if (fCol1) {
				uEnv = uEnd;
			}
			if (fCol1 && '#' == c) {
				fComment = 1;
				continue;
			}

			pcRet = util_ppm_size(& PPMEver, (uEnd|255)+1);
			if ((fCol1 = ('\n' == c))) {
				register char *pcFetch;
				register size_t uLen;

				c = ' ';
				pcRet[uEnd] = '\000';
				if ('$' != pcRet[uEnv] || uEnv+2 > uEnd) {
					/* nada */
				} else if ((char *)0 == (pcFetch = getenv(pcRet+uEnv+1)) || '\000' == *pcFetch) {
					uEnd = uEnv;
				} else {
					uLen = strlen(pcFetch);
					pcRet = util_ppm_size(& PPMEver, ((uLen+uEnv+1)|255)+1);
					(void)strcpy(pcRet+uEnv, pcFetch);
					uEnd = uEnv+uLen;
				}
			}
			pcRet[uEnd++] = c;
			fNeedPad = !isspace(c);
		}
		fclose(fpIn);
		if (uEnd > 0 && isspace(pcRet[uEnd])) {
			--uEnd;
		}
		pcRet = util_ppm_size(& PPMEver, (uEnd|3)+1);
		pcRet[uEnd] = '\000';
		/* BUG: XXX a $ENV on the end of the file w/o a \n on
		 * the end leaves the variable name in place --ksb
		 */
	}
	return pcRet;
}
%%
