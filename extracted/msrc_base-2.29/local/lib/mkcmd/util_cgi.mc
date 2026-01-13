/* $Id: util_cgi.mc,v 8.9 2008/09/05 16:30:05 ksb Exp $
 */

/* Mkcmd interface to decode CGI style args into normal ones		(ksb)
 * Major ideas from Mike MacKenzie (mm).
 */
char **
cgi_args(pargc, pargv)
int *pargc;
char **(*pargv);
{
	static char *apcZero[2] = {(char *)0, (char *)0};
	auto char **argv;
	auto int argc;
	register int i;
	register char *pcParam, *pcSrc, *pcDst;

	/* no params, no problem -- look in an environment variable
	 */
	if ((char **(*))0 != pargv && (char *)0 != (*pargv[0]) && (char *)0 != (pcParam = (*pargv)[1])) {
		/* found them on the command line first */
		/* N.B.: we only take the first word, we toss the rest */
%	} else if ((char *)0 != (pcParam = getenv(%K<cgi_args>1eqv))) %{
		if ('/' == *pcParam)
			++pcParam;
	} else {
		return (char **(*))0 == pargv ? (char **)0 : *pargv;
	}

	/* We have to be able to write on argv to get started:
	 * convert to tab separated words and let envopt argv them.
	 */
	for (pcSrc = pcParam; '\000' != *pcSrc; pcSrc++) {
		if ('&' == *pcSrc) {
			*pcSrc = '\t';
			if (pcSrc > pcParam && '\\' == pcSrc[-1])
				pcSrc[-1] = ' ';
		}
	}
	argc = 1;
	argv = apcZero;
%	argv[0] = %b%;
	(void)u_envopt(pcParam, & argc, & argv);

	/* now de-hex them in place
	 */
	for (i = 1; i < argc; ++i) {
		register int c1, c2;

		for (pcDst = pcSrc = argv[i]; /* switch */; ++pcDst, ++pcSrc) {
			switch (*pcSrc) {
			case '\000':
				*pcDst = '\000';
				break;
			case '+':
				*pcDst = ' ';
				continue;
			case '%':
				c1 = *++pcSrc;
				if (islower(c1))
					c1 = toupper(c1);
				c2 = *++pcSrc;
				if (islower(c2))
					c2 = toupper(c2);
				*pcDst = 16 * (c1 - (isalpha(c1) ? ('A'-10) : '0' )) + (c2 - (isalpha(c2) ? ('A'-10) : '0'));
				continue;
			default:
				*pcDst = *pcSrc;
				continue;
			}
			break;
		}
	}

	/* tell the caller
	 */
	if ((int *)0 != pargc) {
		*pargc = argc;
	}
	if ((char **(*))0 != pargv) {
		*pargv = argv;
	}
	return argv;
}
