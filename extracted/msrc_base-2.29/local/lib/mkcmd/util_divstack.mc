/* $Id: util_divstack.mc,v 1.12 2008/08/25 14:18:18 ksb Exp $
 */
static struct u_dsData {
	unsigned ulen;		/* the lenght of the longest temp string */
	char *pcprefix;		/* from progname or a fixed prefix	*/
	char *pcdiropt;		/* from %qrtL or the like		*/
	char *pcret;		/* space to return any a name		*/
} u_dsData;

/* return -1 for not possible, internal use only			(ksb)
 */
static int
%%K<div_setKey>/ef
{
	if (0 == u_dsData.ulen) {
		errno = ENXIO;
		return -1;
	}
	snprintf(u_dsData.pcret, u_dsData.ulen, "%s_%s", u_dsData.pcprefix, pcKey);
	return 1;
}

/* Output the std version (-V) banner for our module			(ksb)
 */
void
%%K<divVersion>/ef
{
	register const char *pcLevel;
	auto char *pcEos;

%	if ((const char *)0 != (pcLevel = %K<divCurrent>1ev((char *)0))) %{
		(void)strtol(pcLevel, & pcEos, 10);
%		if (0 != strcmp(pcLevel, %K<divLabels>3eqv) && (pcEos == pcLevel || '\000' != *pcEos)) %{
%			fprintf(fpOut, "%%s: cannot parse level string \"%%s\"\n", %b, pcLevel)%;
		}
	}
%	fprintf(fpOut, "%%s: environment prefix \"%%s\"\n", %b, %K<divClient>1ev)%;
%	fprintf(fpOut, "%%s: environment tags: \"%qK<divLabels>1ev\", \"%qK<divLabels>2ev\", \"%qK<divLabels>3ev\", \"%%s\"\n", %b, (const char *)0 == pcLevel ? "1" : pcLevel)%;
}

/* We need to remember the prefix, and the option spelling of		(ksb)
 * the direct option name.  We don't allocate a struct for this dynamically
 * because we've never needed two of them in any application.
 */
unsigned
%%K<divInit>/ef
{
%	register unsigned uLen = %K<divLabels>/e/q/i/sizeof(//+/)+/d0%;

	/* We might put on an integer, usually 1 or 2 digits, allow 31, or
	 * the sum of all the suffixes we might put on.  So any'll fit.
	 */
	if (uLen < 32) {
		uLen = 32;
	}
	if ((char *)0 != u_dsData.pcprefix)
		free((void *)u_dsData.pcprefix);
	if ((char *)0 != u_dsData.pcdiropt)
		free((void *)u_dsData.pcdiropt);
	if ((char *)0 != u_dsData.pcret)
		free((void *)u_dsData.pcret);
	if ((char *)0 == pcName || '\000' == *pcName) {
%		fprintf(stderr, "%%s: null prefix name is invalid\\n", %b)%;
		exit(EX_DATAERR);
	}
	u_dsData.pcprefix = strdup(pcName);
	u_dsData.pcdiropt = (char *)0 != pcSpec ? strdup(pcSpec) : (char *)0;
	uLen += strlen(pcName);
	if (u_dsData.ulen >= uLen)
		return u_dsData.ulen;
	uLen |= 7;
	if ((char *)0 == (u_dsData.pcret = malloc(++uLen))) {
%		fprintf(stderr, "%%s: malloc: %%u: %%s\\n", %b, uLen, %E)%;
		exit(EX_OSERR);
	}
	return u_dsData.ulen = uLen;
}

/* Return the current diversion as a string, might be "d"		(ksb)
 */
char *
%%K<divCurrent>/ef
{
	register char *pcEnv;

%	if (-1 == %K<div_setKey>1ev(%K<divLabels>1eqv)) %{
		errno = ENXIO;
		return pcForce;
	}
	if ((char *)0 == (pcEnv = getenv(u_dsData.pcret))) {
		errno = ENOENT;
		return pcForce;
	}
	return pcEnv;
}

/* Return the diversion forced to be a number, if there is one.		(ksb)
 * -1 error, 0 not one (imply 0) or can't parse it, 1 a good found one
 */
int
%%K<divNumber>/ef
{
	register char *pcDiv;
	auto char *pcEos;

	*puFound = 0;
	if (0 == u_dsData.ulen) {
		errno = ENXIO;
		return -1;
	}
%	if ((char *)0 == (pcDiv = %K<divCurrent>1ev((char *)0))) %{
		return 0;
	}
	*puFound = strtol(pcDiv, &pcEos, 10);
	return pcEos != pcDiv && '\000' == *pcEos;
}

/* Update the link value to the next diversion, install that value	(ksb)
 * -1 for error/failure, 0 for success (you can push a null to unset)
 * $us_2=pcNew, $us_link=2
 */
int
%%K<divPush>/ef
{
	auto unsigned uEncl;
%	auto char acLevel[(sizeof(%K<divLabels>4eqv)|3)+(1+32)]%;

	if (0 == u_dsData.ulen) {
		errno = ENXIO;
		return -1;
	}
%	if (1 != %K<divNumber>1ev(& uEncl)) %{
		uEncl = 0;
	}
%	snprintf(acLevel, sizeof(acLevel), %K<divLabels>4eqv, ++uEncl)%;
%	(void)%K<div_setKey>1ev(acLevel)%;
	if ((const char *)0 == pcNew) {
		unsetenv(u_dsData.pcret);
		return 0;
	}
	if (-1 == setenv(u_dsData.pcret, pcNew, 1)) {
		return -1;
	}
%	(void)%K<div_setKey>1ev(%K<divLabels>1eqv)%;
	return setenv(u_dsData.pcret, acLevel, 1);
}

/* Env out the detached diversion for a direct child (we thinkg)	(ksb)
 * -1 for error/failure, 0 for success, pass (cons char *)0 for the default
 * detached name ("d" in my programs):  $us_d=pcNew
 */
int
%%K<divDetach>/ef
{
	if ((char *)0 == pcTag) {
%		pcTag = %K<divLabels>3eqv%;
	}
%	if (-1 == %K<div_setKey>1ev(pcTag)) %{
		errno = ENXIO;
		return -1;
	}
	if ((const char *)0 == pcNew) {
		unsetenv(u_dsData.pcret);
		return 0;
	}
	return setenv(u_dsData.pcret, pcNew, 1);
}

/* Iterate through the diversions apply a function to each		(ksb)
 * In the order "d", top, top-1, ... 1, 0.  Where 0 is optional.
 */
int
%%K<divSearch>/ef
{
	register char *pcVal;
	register int iRet;
	register unsigned uActive;
	register const char *pcCur;
	auto unsigned uTop;
%	auto char acLevel[(sizeof(%K<divLabels>4eqv)|3)+(1+32)]%;
%	static const char acOOD[] = %K<divLabels>3eqv%;

%	if (-1 == %K<div_setKey>1ev(%K<divLabels>1eqv)) %{
		errno = ENXIO;
		return -1;
	}
	pcCur = getenv(u_dsData.pcret);
%	if (-1 == %K<div_setKey>1ev(acOOD)) %{
		errno = ENXIO;
		return -1;
	}
	if ((char *)0 != (pcVal = getenv(u_dsData.pcret)) && 0 != (iRet = (*pfi)(acOOD, pcVal, (const char *)0 != pcCur && 0 == strcmp(acOOD, pcCur), pvData))) {
		return iRet;
	}
%	if ((char *)0 == pcCur || 1 != %K<divNumber>1ev(& uTop)) %{
		errno = 0;
		return 0;
	}
	uActive = uTop >= iDevDepth ? uTop-iDevDepth : uTop+1;
	for ( ; uTop > 0; --uTop) {
%		snprintf(acLevel, sizeof(acLevel), %K<divLabels>4eqv, uTop)%;
%		(void)%K<div_setKey>1ev(acLevel)%;
		iRet = (*pfi)(acLevel, getenv(u_dsData.pcret), uTop == uActive, pvData);
		if (0 != iRet)
			return iRet;
	}
%	snprintf(acLevel, sizeof(acLevel), %K<divLabels>4eqv, 0)%;
%	(void)%K<div_setKey>1ev(acLevel)%;
	if ((char *)0 == (pcVal = getenv(u_dsData.pcret))) {
		return 0;
	}
	return (*pfi)(acLevel, pcVal, 0 == uActive, pvData);
}

/* Push a result token back into the envionment				(ksb)
 */
int
%%K<divResults>/ef
{
%	if (-1 == %K<div_setKey>1ev(%K<divLabels>2eqv)) %{
		errno = ENXIO;
		return -1;
	}
	if ((const char *)0 == pcValue) {
		unsetenv(u_dsData.pcret);
		return 0;
	}
	return setenv(u_dsData.pcret, pcValue, 1);
}

/* The magic is here, find the diversion the Customer is looking for	(ksb)
 */
char *
%%K<divSelect>/ef
{
	register char *pcOuter, *pcRet;
	register unsigned uOuter;
	auto char *pcEos;

%	pcRet = %K<divClient>2i/%/+/n/ev%;
	if ((char *)0 != pcRet && '-' == pcRet[0] && '\000' == pcRet[1]) {
		pcRet = (char *)0;
	}
	if ((char *)0 != pcRet) {
		return pcRet;
	}

	uOuter = 1;
%	if ((char *)0 == (pcOuter = %K<divCurrent>1ev((char *)0))) %{
		fprintf(stderr, "%s: no enclosing diversion", progname);
%		if (-1 != %K<div_setKey>1ev(%K<divLabels>3eqv) && (char *)0 != (pcOuter = getenv(u_dsData.pcret))) %{
%			fprintf(stderr, ", do you mean to specify %K<divClient>2i/%/+/L/ev as \"%%s\"?", pcOuter)%;
		}
		fprintf(stderr, "\n");
		exit(EX_NOINPUT);
%	} else if (0 != strcmp(pcOuter, %K<divLabels>1eqv)) %{
		uOuter = strtol(pcOuter, &pcEos, 10);
		if (pcEos == pcOuter || '\000' != *pcEos) {
			fprintf(stderr, "%s: cannot parse %s \"%s\" as a number\n", progname, pcOuter, pcOuter);
			exit(EX_DATAERR);
		}
		/* yes we really want to try level 0 --ksb
		 */
		if (uOuter < iDevDepth) {
			fprintf(stderr, "%s: requested depth of -%d is too large for current stack of %u\n", progname, iDevDepth, uOuter);
			exit(EX_DATAERR);
		}
		uOuter -= iDevDepth;
%		if ((char *)0 == (pcRet = %K<divIndex>1ev(uOuter))) %{
			fprintf(stderr, "%s: deversion %u: %s\n", progname, uOuter, strerror(errno));
			exit(EX_DATAERR);
		}
%	} else if (-1 == %K<div_setKey>1ev(%K<divLabels>3eqv)) %{
		fprintf(stderr, "%s: internal buffer snark\n", progname);
		exit(EX_SOFTWARE);
%	} else if ((char *)0 == (pcRet = getenv(u_dsData.pcret))) %{
		fprintf(stderr, "%s: $%s: no diversion set\n", progname, u_dsData.pcret);
		exit(EX_DATAERR);
	}
	return pcRet;
}

/* Get a specific diversion by number (0...N)				(ksb)
 */
char *
%%K<divIndex>/ef
{
%	auto char acIndex[(sizeof(%K<divLabels>4eqv)|3)+(1+32)]%;

%	snprintf(acIndex, sizeof(acIndex), %K<divLabels>4eqv, uLevel)%;
%	if (-1 == %K<div_setKey>1ev(acIndex)) %{
		errno = ENXIO;
		return (char *)0;
	}
	errno = ENOENT;
	return getenv(u_dsData.pcret);
}
