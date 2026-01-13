/* $Id: mode.mc,v 1.1 1999/06/14 14:34:08 ksb Exp $
 * convert a mode to two numbers, manditory bits and optionals		(ksb)
 * this makes a (struct permbits) which we return the address-of.
 * The Implementor may free() this to dispose of it. -- ksb
 *
 * We'll take a mode in octal (0777), and ls-style mode (-rw-r--r-x),
 * an install/instck extended mode (-r?-r?-r--), or a chmod style
 * (a=r,o+w), or any of those separated by a slash.  We are sensitive
 * to the present umask.   We return the bits forced, and those that
 * are "optional".
 *
 * The convention is to include the forced bits always, the optional bits
 * should be used as a mask with any existing mode them or'd into the
 * forced bits.  This is used in "install" and "chmod", only chmod uses the
 * processes umask (complemented of course) to mask the optional bits. --ksb
 */


/* homogeneous versions octal/octal, symbol/symbol, expr/expr
 */

/* Convert an octal mode to an integer					(jms)
 * this OR's the converted mode into the result buffers
 *	mode-forced[/mode-optional]
 */
static int
mode_from_octal(pcLMode, pmMode, pmOpt)
char *pcLMode;		/* the string to convert (e.g., "755")		*/
mode_t *pmMode;		/* the place to put the converted mode		*/
mode_t *pmOpt;		/* the optional bits, always 0			*/
{
	register mode_t mode;	/* mode of installed file		*/
	register int c;		/* next character			*/

	for (mode = 0; '\000' != (c = *pcLMode); ++pcLMode) {
		if (c >= '0' && c <= '7') {
			mode = (mode << 3) + (c - '0');
			continue;
		}
		if ('/' == c) {
			if ((mode_t *)0 != pmMode) {
				*pmMode |= mode;
			}
			mode = 0;
			pmMode = pmOpt;
			continue;
		}
%		fprintf(stderr, "%%s: bad digit in octal mode `%%c\'\n", %b, *pcLMode)%;
		return 1;
	}
	if ((mode_t *)0 != pmMode) {
		*pmMode |= mode;
	}
	return 0;
}


static char mode_acones[] =
	"rwxrwxrwx";		/* mode 0777		*/
/* string to octal mode mask "rwxr-s--x" -> 02751.			(ksb)
 * Note that we modify the string to do this, but we put it back.
 * XXX: this modification of the string is a Bug. -- ksb
 */
static void
mode_from_symbol(pcLMode, pmBits, pmQuests)
char *pcLMode;		/* alpha bits buffer (about 10 chars)		*/
register mode_t *pmBits; /* manditory mode				*/
mode_t *pmQuests;	/* optional mode bits				*/
{
	register int i;
	register mode_t iBit;
	register char *pcBit;
	auto char acModeBuf[64];	/* drwxrwxrwx/d????????? + spew */

	if ((mode_t *)0 != pmBits) {
		*pmBits = 0;
	}
	if ((mode_t *)0 != pmQuests) {
		*pmQuests = 0;
	}
	if ((char *)0 == pcLMode) {
		return;
	}
	pcLMode = strncpy(acModeBuf, pcLMode, sizeof(acModeBuf));

	/* if the mode is long enough to have a file type field check that
	 * out and set the file type bits too
	 */
	switch (i = strlen(pcLMode)) {
	default:
%		(void)fprintf(stderr, "%%s: alphabetical mode must be 9 or 10 characters `%%s\' is %%d\n", %b, pcLMode, i)%;
		exit(EXIT_OPT);
	case 9:
		*pmBits |= S_IFREG;
		break;
	case 10:
		switch (*pcLMode) {
#if HAVE_SLINKS
		case 'l':	/* symbolic link */
			*pmBits |= S_IFLNK;
			break;
#endif	/* no links to think about */
#if defined(S_IFNWK)
		case 'n':	/* network files from HPUX9 */
			*pmBits |= S_IFNWK;
			break;
#endif /* no networked files */
#if defined(S_CDF)
		case 'H':	/* hidden directory, context dependent files*/
			*pmBits |= S_CDF|S_IFDIR;
			break;
#if defined(S_IFIFO)
		case 'p':	/* fifo */
			*pmBits |= S_IFIFO;
			break;
#endif	/* no fifos */
#if defined(S_IFSOCK)
		case 's':	/* socket */
			*pmBits |= S_IFSOCK;
			break;
#endif	/* no sockets */
		case 'b':
			*pmBits |= S_IFBLK;
			break;
		case 'c':
			*pmBits |= S_IFCHR;
			break;
		case '-':
		case 'f':
			*pmBits |= S_IFREG;
			break;
		case 'd':
			*pmBits |= S_IFDIR;
			break;
		default:
%			(void)fprintf(stderr, "%%s: unknown file type `%%c\'\n", %%b, *pcLMode)%;
			exit(EXIT_OPT);
		}
		++pcLMode;
		break;
	}

	/* copy out and turn off set{g,u}id and stick bits
	 * (The upper case bit means no execute underneath.)
	 */
	if ((char *)0 != (pcBit = strchr("sS", pcLMode[2]))) {
		pcLMode[2] = isupper(*pcBit) ? '-' : 'x';
		*pmBits |= S_ISUID;
	}
	if ((char *)0 != (pcBit = strchr("lLsS", pcLMode[5]))) {
		pcLMode[5] = 's' == *pcBit ? 'x' : '-';
		*pmBits |= S_ISGID;
	}
	if ((char *)0 != (pcBit = strchr("tT", pcLMode[8]))) {
		pcLMode[8] = isupper(*pcBit) ? '-' : 'x';
		*pmBits |= S_ISVTX;
	}

	/* read normal mode bits
	 */
	for (i = 0, iBit = 0400; i < 9; ++i, iBit >>= 1) {
		if (pcLMode[i] == mode_acones[i]) {
			*pmBits |= iBit;
		} else if ('-' == pcLMode[i]) {
			continue;
		} else if ('\?' == pcLMode[i]) {
			if ((mode_t *)0 != pmQuests)
				*pmQuests |= iBit;
		} else if ('\000' == pcLMode[i]) {
%			(void)fprintf(stderr, "%%s: not enough bits in file mode\n", %b)%;
		} else if ((char *)0 != strchr("rwxtTsS", pcLMode[i])) {
%			(void)fprintf(stderr, "%%s: bit `%%c\' in file mode is in the wrong column\n", %b, pcLMode[i])%;
			exit(EXIT_OPT);
		} else {
%			(void)fprintf(stderr, "%%s: unknown bit in file mode `%%c\'\n", %b, pcLMode[i])%;
			exit(EXIT_OPT);
		}
	}
}

/* Process modes like chmod (g+r is 0040) (a=X is 0111 or 0000)		(ksb)
 * comma (,) is handled above or here (XXX ZZZ)
 */
static int
mode_from_expr(pcLMode, pmMode, pmOptional)
char *pcLMode;		/* expression [ugoa][+-=][rwxtXsHugo]		*/
mode_t mMode;		/* maditory mode bits				*/
mode_t mOptional;	/* optional mode bits				*/
{
	register mode_t iAllow, iQuest, iReset, iArg;
	register int cOp, iSaw;

	iQuest = iAllow = iSaw = 0;
	while ('\000' != *pcLMode) { switch (*pcLMode++) {
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
		iAllow |= 01007;
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

		/* XXX we could take a trap here and in the handler
		 * use the wrong mode, but 022 is not all that bad
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

	while ('\000' != *pcLMode) { switch (*pcLMode++) {
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
	case 'X':
		if (S_IFDIR == (*pmMode & S_IFMT) || 0 != (00111 & *pmMode)) {
			iArg |= 00111;
		}
		continue;
	case 'u':
		mTemp = *pmMode & 00700;
		iArg |= mTemp | (mTemp >> 3) | (mTemp >> 6);
		continue;
	case 'g':
		mTemp = *pmMode & 00070;
		iArg |= (mTemp << 3) | mTemp | (mTemp >> 3);
		continue;
	case 'o':
		mTemp = *pmMode & 00007;
		iArg |= (mTemp << 3) | (mTemp << 3) | mTemp;
		continue;
	case 'a':
		iArg |= *pmMode & 00777;
		continue;
	default:
		break;
	} break; }

	switch (cOp) {
	case '-':
		*pmMode &= ~(iArg & iAllow);
		break;
	case '=':
		*pmMode &= ~iReset;
		/* fall through */
	case '+':
		*pmMode |= iArg & iAllow;
		break;
	}
	/* when you depended on the present umask the bits left-over
	 * are optional -- ksb
	 */
	if ((mode_t *)0 != pmOptional) {
		pmOptional |= iArg & iQuest;
	}

	if (',' == *pcLMode) {
		return mode_from_expr(pcLMode+1, pmMode, pmOptional);
	}
	if ('/' == *pcLMode) {
		return mode_from_expr(pcLMode+1, pmOptional, (mode_t *)0);
	}
	return '\000' != *pcLMode;
}


/* extra bits in the bit listing are handled strangely			(ksb)
 * worse if we want to do "rw-l--r--" (the "l").  We don't, "s" is OK.
 */
static void
mode_ebit(pcBit, bSet, cSet, cNotSet)
char *pcBit;		/* pointer the the alpha bit			*/
int bSet;		/* non-zero means it is set			*/
int cSet;		/* overstrike with this				*/
int cNotSet;		/* it should have been this, else upper case it	*/
{
	if (0 != bSet) {
		*pcBit = (cNotSet == *pcBit) ? cSet : toupper(cSet);
	}
}


/* Convert a string that ls(1) understands into something that	(jms&ksb)
 * chmod(2) understands (extended), or an octal mode.
 ! The string we are given may be in the text (const) segment,
 ! we have to copy it to a write segment to allow StrMode to work.
 *
 * Optional bits may be specified after a `/' as in
 *	0711/044
 * which allows any of {0711, 0751, 0715, 0755}, or write symolicly
 *	-rwx--x--x/----r--r--
 * or a symbolic mode may contain a `?' in place of a bit for optional:
 *	-rwx?-x?-x
 * (which is quite readable).
 *
 * The slash notaion is needed for optional setgid directories, at least.
 */
void
mode_cvt(pcLMode, pmMode, pmQuest)
char *pcLMode;		/* the string to convert (e.g., "755")		*/
mode_t *pmMode;		/* the place to put the converted mode		*/
mode_t *pmQuest;	/* the place to put the converted optional mode	*/
{
	register char *pcScan;	/* copy mode so we can write on it	*/
	auto char acDown[MAXPATHLEN+1];	/* where we write		*/

	if ((mode_t *)0 != pmMode)
		*pmMode = 0;
	if ((mode_t *)0 != pmQuest)
		*pmQuest = 0;

	pcScan = acDown;
	while ('/' != *pcLMode && '\000' != *pcLMode) {
		*pcScan++ = *pcLMode++;
	}
	*pcScan = '\000';

	if (isdigit(*acDown)) {
		mode_octal(acDown, pmMode, pcQuest);
	} else {
		mode_symbol(acDown, pmMode, pmQuest);
	}

	if ('/' == *pcLMode && (int *)0 != pmQuest) {
		++pcLMode;
		(void)strncpy(acDown, pcLMode, sizeof(acDown));
		if (isdigit(*acDown)) {
			mode_octal(acDown, pmQuest, pcQuest);
		} else {
			mode_symbol(acDown, pmQuest, pmQuest);
		}
	}
}


/*@Explode mode_str@*/
/* convert a MODE into a symbolic format				(ksb)
 */
char *
mode_to_symbol(pcLMode, mMode, mOptional)
char *pcLMode;		/* alpha bits buffer (10 chars or more)		*/
mode_t mMode;		/* maditory mode bits				*/
mode_t mOptional;	/* optional mode bits				*/
{
	register int i;
	register mode_t iBit;

	(void)strcpy(pcLMode, mode_acones);
	for (i = 0, iBit = 0400; i < 9; ++i, iBit >>= 1) {
		if (0 == (mMode & iBit))
			pcLMode[i] = '-';
	}
	mode_ebit(pcLMode+2, S_ISUID & mMode, 's', 'x');
	mode_ebit(pcLMode+5, S_ISGID & mMode, 's', 'x');
	mode_ebit(pcLMode+8, S_ISVTX & mMode, 't', 'x');
	for (i = 0, iBit = 0400; i < 9; ++i, iBit >>= 1) {
		if (0 == (mOptional & iBit))
			continue;
		if ('-' == pcLMode[i])
			pcLMode[i] = '\?';
	}
	return pcLMode;
}

/* Convert a text representation into a mode structure			(ksb)
 * free it when you are done with it.  Pass the Customer string and the
 * source of the data (viz %N, "%qL").
 * The more primative routines expect homogeneous data (octal/octal) this
 * routine can take "symbols/octal" for the rare psyco-UNIX-Dude. -- ksb
 */
/* public interface */
%struct %K<mode_struct>2v *
%%K<mode_cvt>1v(pcMode, pcFrom)
char *pcMode, *pcFrom;
{
	register char *pcSlash;
%	register struct %K<mode_struct>2v *u_pMPRet%;

%	if ((struct %K<mode_struct>2v *)0 == (u_pMPRet = calloc(1, sizeof(struct %K<mode_struct>2v)))) %{
		return (void *)0;
	}
	u_pMPRet->imand = 0;
	u_pMPRet->iopt = 0;
	u_pMPRet->ioff = 0;
	u_pMPRet->itype = 0;

	/* nothing to convert, or a empty string.  Allocated space
	 * returned to caller to fill-in.
	 */
	if ((char *)0 == pcMode || '\000' == pcMode[0]) {
		return u_pMPRet;
	}

	/* This allows some strange combinations of ls modes and octal,
	 * but we are "liberal with what we accept and conservative with
	 * what we ouptut" -- ksb
	 */
	switch (*pcMode) {	/* same as ls, or find (mostly) */
#if defined(S_IFDIR)
	case 'd':
		++pcMode;
		u_pMPRet->itype = S_IFDIR;
		break;
#endif
#if defined(S_IFBLK)
	case 'b':
		++pcMode;
		u_pMPRet->itype = S_IFBLK;
		break;
#endif
#if defined(S_IFCHR)
	case 'c':
		++pcMode;
		u_pMPRet->itype = S_IFCHR;
		break;
#endif
#if defined(S_IFIFO)
	case 'p':
		++pcMode;
		u_pMPRet->itype = S_IFIFO;
		break;
#endif
#if defined(S_IFLNK)
	case 'l':
		++pcMode;
		u_pMPRet->itype = S_IFLNK;
		break;
#endif
#if defined(S_CDF)
	case 'H':	/* HPUX context depend file, sigh */
		++pcMode;
		u_pMPRet->itype = S_FDIR|S_CDF;
		break;
#endif
#if defined(S_IFNWK)
	case 'n':	/* HPUX, but what is this for */
		++pcMode;
		u_pMPRet->itype = S_IFNWK;
		break;
#endif
#if defined(S_IFSOCK)
	case 's':
		++pcMode;
		u_pMPRet->itype = S_IFSOCK;
		break;
#endif
#if defined(S_IFCTG)
	case 'm': /* MASSCOMP, but what does ls show for one XXX --ksb */
		++pcMode;
		u_pMPRet->itype = S_IFCTG;
		break;
#endif
#if defined(S_IFWHT)
	case 'w': /* FREEBSD whiteout hides a union'd file below -- ksb */
		/* can you stat(2) one of these? */
		++pcMode;
		u_pMPRet->itype = S_IFWHT;
		break;
#endif
	case '-': /* "-r" is "turn off read" XXX */
		if ('-' == pcMode[1] || 'r' == pcMode[1] || isdigit(pcMode[1])) {
	case 'f':
			++pcMode;
		}
	default:
		u_pMPRet->itype = S_IFREG;
		break;
	}
	/* find the '/' or the end */
	/* convert imand, iopt */
	/* convert iopt */
	/* convert chmod/umask style */
	return u_pMPRet;
}
