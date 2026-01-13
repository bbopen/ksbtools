/* $Id: util_mult.mc,v 8.9 1997/10/10 21:42:10 ksb Beta $
 */

/* do the third prec level						(ksb)
 * ops: (), number, scales
 */
static unsigned long
pm_term(ppcIn, pPM, uiCount)
char **ppcIn;
struct PMnode *pPM;
unsigned int uiCount;
{
	register unsigned long ulTerm;
	register unsigned int u;
	auto char *pcOut;

	pcOut = *ppcIn;
	switch (*pcOut) {
	case '(':
		++pcOut;
%		ulTerm = %K<pm_expr>1ev(& pcOut, pPM, uiCount)%;
		if (')' != *pcOut) {
			/* missing close paren */
			*ppcIn = pcOut;
			return ulTerm;
		}
		++pcOut;
		break;
	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
	case '8': case '9':
		ulTerm = strtoul(pcOut, & pcOut, 0);
		break;
	default:
		ulTerm = 1;
		break;
	}
	while ('\000' != *pcOut) {
		if (isspace(*pcOut)) {
			++pcOut;
			continue;
		}
		if (isdigit(*pcOut)) {
			ulTerm *= strtoul(pcOut, & pcOut, 0);
			continue;
		}
		if ((char *)0 != strchr("*/%+-)", *pcOut)) {
			break;
		}
		for (u = 0; u < uiCount; ++u) {
			if (*pcOut == pPM[u].ckey) {
				break;
			}
		}
		if (u != uiCount) {
			ulTerm *= pPM[u].umult;
			++pcOut;
			continue;
		}
		/* unknown scale letter */
		if (isalpha(*pcOut)) {
%			%H/"scale"/H/"unknown unit"/K<pm_malformed>/ef%;
			++pcOut;
		}
		break;
	}
	*ppcIn = pcOut;
	return ulTerm;
}

/* do the second prec level						(ksb)
 * ops: *, /, %
 */
static unsigned long
pm_factor(ppcIn, pPM, uiCount)
char **ppcIn;
struct PMnode *pPM;
unsigned int uiCount;
{
	register unsigned long ulProd, ulTmp;
	auto char *pcOut;

	ulProd = pm_term(ppcIn, pPM, uiCount);
	pcOut = *ppcIn;
	while ('\000' != *pcOut && ')' != *pcOut) {
		if (isspace(*pcOut)) {
			++pcOut;
			continue;
		}
		if ('*' == *pcOut) {
			++pcOut;
%			ulProd *= %K<pm_expr>1ev(& pcOut, pPM, uiCount)%;
			continue;
		}
		if ('(' == *pcOut) {
			ulProd *= pm_term(& pcOut,  pPM, uiCount);
			continue;
		}
		if ('/' == *pcOut) {
			ulTmp = pm_term(& pcOut,  pPM, uiCount);
			if (0L == ulTmp) {
%				%H/"expression"/H/"modulo by zero"/K<pm_malformed>/ef%;
				continue;
			}
			ulProd /= ulTmp;
			continue;
		}
		if ('%' == *pcOut) {
			ulTmp = pm_term(& pcOut,  pPM, uiCount);
			if (0L == ulTmp) {
%				%H/"expression"/H/"modulo by zero"/K<pm_malformed>/ef%;
				continue;
			}
			ulProd %= ulTmp;
			continue;
		}
		break;
	}
	*ppcIn = pcOut;
	return ulProd;
}

/* Do the conversion work						(ksb)
 * We even do (non-prec) 5 banger math, like: '(5+3)*2k'
 * (also handle the implicit multiplication "7(2+11)m")
 */
unsigned long
%%K<pm_expr>1ev(ppcIn, pPM, uiCount)
char **ppcIn;
struct PMnode *pPM;
unsigned int uiCount;
{
	register unsigned long ulRet, ulSign;
	auto char *pcOut;

	ulRet = pm_factor(ppcIn, pPM, uiCount);
	pcOut = *ppcIn;
	ulSign = 1;
	while ('\000' != *pcOut && ')' != *pcOut) {
		if (isspace(*pcOut)) {
			++pcOut;
			continue;
		}
		/* look sign of addition
		 */
		switch (*pcOut) {
		case '+':
			++pcOut;
			continue;
		case '-':
			ulSign *= -1;
			++pcOut;
			continue;
		default:
			break;
		}
		ulRet += ulSign * pm_factor(& pcOut, pPM, uiCount);
		ulSign = 1;
	}
	*ppcIn = pcOut;
	return ulRet;
}

/* clean call from mkcmd to do scaled units				(ksb)
 * also has a nice (kinda) builtin help facility
 */
unsigned long
%%K<pm_cvt>1ev(pcRaw, pPM, uiCount, pcCaller)
char *pcRaw, *pcCaller;
struct PMnode *pPM;
unsigned int uiCount;
{
	register unsigned long ulRet;
	register char *pcQuesty;
	register struct PMnode *pPMUnit;
	auto unsigned int uC1 = 1, uC2 = 1, uLen, u;
	auto char acGuess[132];

	if ((char *)0 == (pcQuesty = strchr(pcRaw, '?'))) {
%		ulRet = %K<pm_expr>1ev(& pcRaw, pPM, uiCount);
		if ('\000' != *pcRaw) {
			/* there was junk on the end? */
%			%H/pcCaller/H/pcRaw/K<pm_malformed>/ef%;
		}
		return ulRet;
	}

	/* help?
	 */
	if (pcQuesty == pcRaw) {
		++pcQuesty;
		ulRet = 0L;
	} else {
		*pcQuesty = '\000';
%		ulRet = %K<pm_expr>1ev(& pcRaw, pPM, uiCount);
		if ('\000' != *pcRaw) {
			/* there was junk on the end? */
%			%H/pcCaller/H/pcRaw/K<pm_malformed>/ef%;
		}
		*pcQuesty++ = '?';
	}

	/* xx?fmt
	 */
	if ('\000' == *pcQuesty) {
		pcQuesty = "%lu";
	}
	pPMUnit = (struct PMnode *)0;
	for (u = 0; u < uiCount; ++u) {
		if ('\000' == pPM[u].ckey) {
			pPMUnit = pPM+u;
		}
		uLen = strlen(pPM[u].pctext);
		if (uLen > uC1) {
			uC1 = uLen;
		}
		(void)sprintf(acGuess, pcQuesty, pPM[u].umult);
		uLen = strlen(acGuess);
		if (uLen > uC2) {
			uC2 = uLen;
		}
	}

%	printf("%%s: %%s conversion table", %b, (struct PMnode *)0 == pPMUnit || (char *)0 == pPMUnit->pctext ? "unit" : pPMUnit->pctext)%;
	if (0 != ulRet) {
		printf(" for ");
		printf(pcQuesty, ulRet);
		printf(":\n");
	} else {
		printf("\n");
	}
	for (u = 0; u < uiCount; ++u) {
		register unsigned long ulTemp;

		if (pPMUnit == pPM+u) {
			continue;
		}
		(void)sprintf(acGuess, pcQuesty, pPM[u].umult);
		printf(" %c %*s %*s", pPM[u].ckey, uC1, pPM[u].pctext, uC2, acGuess);
		if (0 != ulRet && 0 != (ulTemp = ulRet/pPM[u].umult)) {
			(void)sprintf(acGuess, pcQuesty, ulTemp);
			printf(" %s%c", acGuess, pPM[u].ckey);
			if (0 != (ulTemp = (ulRet % pPM[u].umult))) {
				(void)sprintf(acGuess, pcQuesty, ulTemp);
				printf("+%s", acGuess);
			}
		}
		printf("\n");
	}
	return ulRet;
}
