/* $Id: mode.mc,v 8.4 2008/09/05 16:30:05 ksb Exp $
 */


/* convert an octal mode to an integer					(jms)
 */
static void
u_mode_octal(pcLMode, pmMode, pcName)
char *pcLMode;		/* the string to convert (e.g., "755")		*/
mode_t *pmMode;		/* the place to put the converted mode		*/
char *pcName;
{
	register mode_t mode;	/* mode of installed file		*/
	register int c;		/* next character			*/

	for (mode = 0; '\000' != (c = *pcLMode); ++pcLMode) {
		if (c >= '0' && c <= '7') {
			mode = (mode << 3) + (c - '0');
			continue;
		}
%		(void)fprintf(stderr, "%%s: %%s: bad digit int mode `%%c\'\n", %b, pcName, *pcLMode)%;
		exit(10);
	}
	if ((mode_t *)0 != pmMode) {
		*pmMode |= mode;
	}
}

/* Extra bits in the bit listing are handled strangely			(ksb)
 */
static void
u_mode_ebit(pcBit, mSet, cSet, cNotSet)
char *pcBit;		/* pointer the the alpha bit			*/
mode_t mSet;		/* should it set over struck			*/
int cSet;		/* overstrike with this				*/
int cNotSet;		/* it should have been this, else upper case it	*/
{
	if (0 != mSet) {
		*pcBit = (cNotSet == *pcBit) ? cSet : toupper(cSet);
	}
}

/* Process modes like chmod (g+r is 0040) (a=X is 0111 or 0000)		(ksb)
 * comma (,) is handled here.  This doesn't always do what I'd want.
 */
static int
u_mode_expr(pcLMode, pMC)
char *pcLMode;		/* expression [ugoa][+-=][rwxtXsHugo]		*/
u_MODE_CVT *pMC;
{
	register mode_t iAllow, iQuest, iReset, iArg;
	register int cOp, iSaw;

	iQuest = iAllow = iSaw = 0;
	for (/*param*/; '\000' != *pcLMode; ++pcLMode) { switch (*pcLMode) {
	case ' ':	/* liberally eat comma, space, tab */
	case ',':
	case '\t':
		continue;
	case 'a':
		iAllow |= 07777;
		iReset |= 00777;
		iSaw = 1;
		continue;
	case 'u':
		iAllow |= 04700;
		iReset |= 00700;
		iSaw = 1;
		continue;
	case 'g':
		iAllow |= 02070;
		iReset |= 00070;
		iSaw = 1;
		continue;
	case 'o':
		iAllow |= 00007;	/* o+t doesn't, sigh! */
		iReset |= 00007;
		iSaw = 1;
		continue;
	case '+':
	case '-':
	case '=':
		break;
	default:
		return 1;
	} break; }

	/* empty expression or malformed one, get out
	 */
	if ('\000' == *pcLMode) {
		return iSaw;
	}

	/* no target maps to process umask
	 */
	if (0 == iSaw) {
		register mode_t iUmask;

		/* XXX we could take a trap here and into a sig handler
		 * use the wrong mask, but 022 is not all that bad
		 * and the cost of doing the signal work takes longer
		 * than bouncing in/out of kernel space -- there really
		 * should be a "give me 2 syscalls" call (well, three).
		 */
		iUmask = umask(0022);
		(void)umask(iUmask);
		iAllow = (07777 & ~iUmask) | 01000;	/* +t == a+t */
		iQuest = 00777 & iUmask;
		iReset = 00777;
	}

	switch (cOp = *pcLMode++) {
	case '+':
	case '-':
	case '=':
		break;
	default:
		return 1;
	}

	for (/*inv.*/; '\000' != *pcLMode; ++pcLMode) { switch (*pcLMode) {
		register mode_t mTemp;
	case 'r':
		iArg |= 00444;
		continue;
	case 'w':
		iArg |= 00222;
		continue;
	case 'x':
		iArg |= 00111;
		continue;
	case 'H':
#if defined(S_CDF)
		iArg |= S_CDF;
#else
		iArg |= 04000;
#endif
		continue;
	case 't':
		iArg |= 01000;
		iAllow |= 01000;
		continue;
	case 's':
		iArg |= 06000;
		continue;
	case 'R':
		if (S_IFDIR == (pMC->iset & S_IFMT) || 0 != (00444 & pMC->iset)) {
			iArg |= 00444;
		}
		continue;
	case 'W':
		if (S_IFDIR == (pMC->iset & S_IFMT) || 0 != (00222 & pMC->iset)) {
			iArg |= 00222;
		}
		continue;
	case 'X':
		if (S_IFDIR == (pMC->iset & S_IFMT) || 0 != (00111 & pMC->iset)) {
			iArg |= 00111;
		}
		continue;
	case 'u':
		mTemp = pMC->iset & 00700;
		iArg |= mTemp | (mTemp >> 3) | (mTemp >> 6);
		continue;
	case 'g':
		mTemp = pMC->iset & 00070;
		iArg |= (mTemp << 3) | mTemp | (mTemp >> 3);
		continue;
	case 'o':
		mTemp = pMC->iset & 00007;
		iArg |= (mTemp << 3) | (mTemp << 3) | mTemp;
		continue;
	case 'a':
		iArg |= pMC->iset & 00777;
		continue;
	default:
		break;
	} break; }

	switch (cOp) {
	case '-':
		pMC->iset &= ~(iArg & iAllow);
		break;
	case '=':
		pMC->iset &= ~iReset;
		/* fall through */
	case '+':
		pMC->iset |= iArg & iAllow;
		break;
	}
	/* when you depended on the present umask the bits left-over
	 * are optional -- ksb
	 */
	pMC->ioptional |= iArg & iQuest;

	if (',' == *pcLMode) {
		auto u_MODE_CVT MCRecur;
		MCRecur.iset = 0;
		MCRecur.ioptional = 0;
		iSaw = u_mode_expr(pcLMode+1, &MCRecur);
		pMC->iset |= MCRecur.iset;
		pMC->ioptional |= MCRecur.ioptional;
		return iSaw;
	}
	return '\000' != *pcLMode;
}

/* try to guess a stat format for the letter presented			(ksb)
 */
static int
mode_ifmt(int cFmt, mode_t *pmFmt)
{
	switch (cFmt) {
#if HAVE_SLINKS
	case 'l':	/* symbolic link */
		*pmFmt = S_IFLNK;
		break;
#endif	/* no links to think about */
#if defined(S_IFIFO)
	case 'p':	/* fifo */
		*pmFmt = S_IFIFO;
		break;
#endif	/* no fifos */
#if defined(S_IFDOOR)
	case 'D':
		*pmFmt = S_IFDOOR;
		break;
#endif
#if defined(S_IFSOCK)
	case 's':	/* socket */
		*pmFmt = S_IFSOCK;
		break;
#endif	/* no sockets */
	case 'b':
		*pmFmt = S_IFBLK;
		break;
	case 'c':
		*pmFmt = S_IFCHR;
		break;
	case '-':
	case 'f':
		*pmFmt = S_IFREG;
		break;
	case 'd':
		*pmFmt = S_IFDIR;
		break;
	default:
		return 1;
	}
	return 0;
}

static char u_mode_acOnes[] =
	"rwxrwxrwx";		/* mode 0777	*/
/*
 * string to octal mode mask "rwxr-s--x" -> 02751.			(ksb)
 * Note that we modify the string to do this, but we put it back.
 */
static int
u_mode_symbol(pcLMode, pMC)
char *pcLMode;		/* alpha bits buffer (about 10 chars)		*/
u_MODE_CVT *pMC;	/* output buffer, from calloc below		*/
{
	register int iBit, i;
	register char *pcBit;

	for (i = 0; '\000' != pcLMode[i]; ++i) {
		if ((char *)0 == strchr(" \taugo+RWX=,", pcLMode[i]))
			continue;
		return u_mode_expr(pcLMode, pMC);
	}

	/* if the mode is long enough to have a file type field check that
	 * out and set the file type bits too
	 */
	switch (i) {
	default:
		return u_mode_expr(pcLMode, pMC);
	case 9:
		pMC->iifmt = S_IFREG;
		break;
	case 10:
		pMC->cfmt = *pcLMode;
		if (0 != mode_ifmt(*pcLMode, & pMC->iifmt)) {
			pMC->cfmt = '\000';
			return u_mode_expr(pcLMode, pMC);
		}
		++pcLMode;
		break;
	}

	/* copy out and turn off set{g,u}id and stick bits
	 * (The upper case bit means no execute underneath.)
	 */
	if ((char *)0 != (pcBit = strchr("sS", pcLMode[2]))) {
		pcLMode[2] = isupper(*pcBit) ? '-' : 'x';
		pMC->iset |= S_ISUID;
	}
	if ((char *)0 != (pcBit = strchr("lLsS", pcLMode[5]))) {
		pcLMode[5] = 's' == *pcBit ? 'x' : '-';
		pMC->iset |= S_ISGID;
	}
	if ((char *)0 != (pcBit = strchr("tT", pcLMode[8]))) {
		pcLMode[8] = isupper(*pcBit) ? '-' : 'x';
		pMC->iset |= S_ISVTX;
	}

	/* read normal mode bits
	 */
	for (i = 0, iBit = 0400; i < 9; ++i, iBit >>= 1) {
		if (pcLMode[i] == u_mode_acOnes[i]) {
			pMC->iset |= iBit;
		} else if ('-' == pcLMode[i]) {
			continue;
		} else if ('\?' == pcLMode[i]) {
			pMC->ioptional |= iBit;
		} else if (isupper(pcLMode[i]) && tolower(pcLMode[i]) == u_mode_acOnes[i]) {
			pMC->ioptional |= iBit;
		} else if ('\000' == pcLMode[i]) {
%			(void)fprintf(stderr, "%%s: not enough bits in file mode\n", %b)%;
		} else if ((char *)0 != strchr("rwxtTsS", pcLMode[i])) {
%			(void)fprintf(stderr, "%%s: bit `%%c\' in file mode is in the wrong column\n", %b, pcLMode[i])%;
			exit(10);
		} else {
%			(void)fprintf(stderr, "%%s: unknown representation bit in file mode `%%c\'\n", %b, pcLMode[i])%;
			exit(10);
		}
	}

	/* here we put the set{u,g}id and sticky bits back
	 */
	u_mode_ebit(pcLMode+2, S_ISUID & pMC->iset, 's', 'x');
	u_mode_ebit(pcLMode+5, S_ISGID & pMC->iset, 's', 'x');
	u_mode_ebit(pcLMode+8, S_ISVTX & pMC->iset, 't', 'x');
	return 0;
}


/*
 * Convert a string that ls(1) understands into something that	     (jms&ksb)
 * chmod(2) understands (extended), or an octal mode.
 ! The string we are given may be in the text (const) segment,
 ! we have to copy it to a write segment to allow u_mode_symbol to work.
 *
 * Optional bits may be specified after a `/' as in
 *	0711/044
 * which allows any of {0711, 0751, 0715, 0755}, or write symbolicly
 *	-rwx--x--x/----r--r--
 * or a symbolic mode may contain a `?' in place of a bit for optional:
 *	-rwx?-x?-x
 * (which is quite readable).
 *
 * The slash notaion is needed for optional setgid directories, at least.
 */
void
%%K<mode_cvt>1v(pcLMode, pMC, pcName)
char *pcLMode;		/* the string to convert (e.g., "755")		*/
u_MODE_CVT *pMC;
char *pcName;
{
	register char *pcScan;	/* copy mode so we can write on it	*/
	auto mode_t mLook;
	auto char acDown[MAXPATHLEN+1];	/* where we write		*/

	pMC->iset = 0;
	pMC->ioptional = 0;
	pMC->iifmt = 0;
	pMC->cfmt = '\000';

	if ((char *)0 == pcLMode) {
		return;
	}
	pcScan = acDown;
	while ('/' != *pcLMode && '\000' != *pcLMode) {
		*pcScan++ = *pcLMode++;
	}
	*pcScan = '\000';

	pMC->iifmt = S_IFREG;
	if (0 == mode_ifmt(acDown[0], & mLook) && isdigit(acDown[1])) {
		pMC->cfmt = acDown[0];
		pMC->iifmt = mLook;
		u_mode_octal(acDown+1, & pMC->iset, pcName);
	} else if (isdigit(*acDown)) {
		u_mode_octal(acDown, & pMC->iset, pcName);
	} else if (u_mode_symbol(acDown, pMC)) {
%		fprintf(stderr, "%%s: %%s: %%s: unparsable\n", %b, pcName, acDown)%;
		exit(8);
	}

	if ('/' == *pcLMode) {
		auto u_MODE_CVT MCLocal;

		MCLocal.iset = 0;
		MCLocal.ioptional = 0;
		MCLocal.iifmt = 0;
		MCLocal.cfmt = '\000';
		(void)strcpy(acDown, pcLMode+1); /* need a writable copy */
		if (isdigit(*acDown)) {
			u_mode_octal(acDown, & MCLocal.iset, pcName);
			pMC->ioptional |= MCLocal.iset;
		} else if (u_mode_symbol(acDown, & MCLocal)) {
%			fprintf(stderr, "%%s: %%s: %%s: optionals unparsable\n", %b, pcName, acDown)%;
			exit(8);
		} else {
			if ('\000' != MCLocal.cfmt) {
				pMC->cfmt = MCLocal.cfmt;
				pMC->iifmt = MCLocal.iifmt;
			}
			pMC->ioptional |= MCLocal.iset|MCLocal.ioptional;
		}
	}
}

/* Setup to process all the mode's the Implementor declared.		(ksb)
 */
static void
u_mode_init()
{
	register u_MODE_CVT *pMCMem;

%	if ((u_MODE_CVT *)0 == (pMCMem = (u_MODE_CVT *)calloc(%K<u_mode_init>j, sizeof(u_MODE_CVT)))) %{
%		fprintf(stderr, "%%s: calloc: %%d: no core\n", %b, %K<u_mode_init>j)%;
		exit(61);
	}
%@foreach "u_mode_init"
%	%n = pMCMem++%;
%@endfor
}
