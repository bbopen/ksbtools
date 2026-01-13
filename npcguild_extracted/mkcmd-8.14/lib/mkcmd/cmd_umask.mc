/* $Id: cmd_umask.mc,v 8.9 1999/06/12 00:43:58 ksb Exp $
 */

/* decode a chmod style mask (ugoa +-= rwxstHXugo ]			(ksb)
 */
static mode_t
u_DecodeMask(pcOrig, iBase)
char *pcOrig;
mode_t iBase;
{
	register mode_t iOut;
	register char cWhich;
	register char *pcMask;

	iOut = iBase;
	for (pcMask = pcOrig; '\000' != *pcMask; ++pcMask) {
		register mode_t iAllow = 00000, iArg = 00000;
		register mode_t iReset = 00000, iSet;

		for (/*none */; /*1*/; ++pcMask) { switch (*pcMask) {
		case 'a':
			iAllow |= 07777;
			iReset |= 00777;
			continue;
		case 'u':
			iAllow |= 04700;
			iReset |= 00700;
			continue;
		case 'g':
			iAllow |= 02070;
			iReset |= 00070;
			continue;
		case 'o':
			iAllow |= 00007;
			iReset |= 00007;
			continue;
		default:
			break;
		} break ; }

		switch (cWhich = *pcMask++) {
		case '+':
		case '-':
		case '=':
			break;
		default:
%			fprintf(stderr, "%%s: unknown mode operator `%%c' (in `%%s')\n", %b, cWhich, pcOrig)%;
			return iBase;
		}

		for (; '\000' != *pcMask; ++pcMask) { switch (*pcMask) {
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
			iArg |= 04000;
			continue;
		case 's':
			iArg |= 06000;
			iAllow |= 06000;
			continue;
		case 't':
			iArg |= 01000;
			iAllow |= 01000;
			continue;
		case 'X':
			if (0 != (00111 & iBase)) {
				iArg |= 00111;
			}
			continue;

		/* from original file
		 */
		case 'u':
			iSet = iBase & 00700;
			iArg |= iSet | (iSet >> 3) | (iSet >> 6);
			continue;
		case 'g':
			iSet = iBase & 00070;
			iArg |= (iSet << 3) | iSet | (iSet >> 3);
			continue;
		case 'o':
			iSet = iBase & 00070;
			iArg |= (iSet << 6) | (iSet << 3) | iSet;
			continue;

		/* from what?
		 */
		default:
%			fprintf(stderr, "%%s: umask bit `%%c' unknown (in %%s)\n", %b, *pcMask, pcOrig)%;
			return iBase;
		} break; }

		/* not sure about this LLL (ksb)
		 */
		if (0 == iAllow) {
			iAllow = iReset = 00777;
		}
		iArg &= iAllow;
		switch (cWhich) {
		case '-':
			iOut &= ~iArg;
			break;
		case '=':
			iOut &= ~iReset;
			/* fall thought */
		case '+':
			iOut |= iArg;
			break;
		}

		if (',' != *pcMask)
			break;
	}
	return iOut;
}

/* change this process's umask						(ksb)
 */
static int
cmd_umask(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register mode_t iMask;
	register char *pcCmdname, *pcParam;

	pcCmdname = argv[0];

	iMask = umask(0);
	(void)umask(iMask);

	if (1 == argc) {
		printf("%o\n", iMask);
		return 0;
	}
	if (2 != argc) {
%		fprintf(stderr, "%%s: %%s: usage [mask]\n", %b, pcCmdname)%;
#if NEED_SYMBOLIC_UMASK
%		fprintf(stderr, "%%s: %%s: usage -S\n", %b, pcCmdname)%;
#endif
		return 1;
	}

	pcParam = argv[1];
#if NEED_SYMBOLIC_UMASK
	/* like u=rwx,g=rx,o=rx
	 */
	if ('-' == pcParam[0] && 'S' == pcParam[1] && '\000' == pcParam[2]) {
		register int i;
		static char _e[] = "";
		for (i = 0; i < 3; ++i) {
			printf("%c=%s%s%s%s", "ugo"[i],
				0 == (iMask & 0400) ? "r" : _e,
				0 == (iMask & 0200) ? "w" : _e,
				0 == (iMask & 0100) ? "x" : _e,
				2 == i ? "\n" : ",");
			iMask <<= 3;
		}
		return 0;
	}
#endif
	if (pcParam[0] >= '0' && '7' >= pcParam[0]) {
		iMask = 0;
		while ('\000' != *pcParam) {
			if (pcParam[0] < '0' || '7' < pcParam[0]) {
%				fprintf(stderr, "%%s: %%s: bad octal digit '%%c' (in %%s)\n", %b, pcCmdname, *pcParam, argv[1])%;
				return 2;
			}
			iMask *= 8;
			iMask += *pcParam++ - '0';
		}
	} else {
		/* [ugoa]+-=[rwxtsXHugo]
		 */
		iMask = ~u_DecodeMask(pcParam, ~iMask);
	}

	if (-1 == umask(iMask)) {
%		fprintf(stderr, "%%s: %%s: umask: %%0o: %%s\n", %b, pcCmdname, iMask, %E)%;
		return 16;
	}
	return 0;
}
