/* $Id: mkman.c,v 8.2 1997/10/18 17:56:20 ksb Exp $
 * Based on some ideas from "mkprog", this program generates		(ksb)
 * a manual page from the command line data
 * See the README file for usage of this program & the man page.
 *	$Compile: ${CC-cc} -c ${DEBUG--g} -I/usr/include/local %f
 */
#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>

#include "machine.h"
#include "main.h"
#include "type.h"
#include "option.h"
#include "scan.h"
#include "parser.h"
#include "mkusage.h"
#include "list.h"
#include "mkcmd.h"
#include "atoc.h"
#include "key.h"
#include "emit.h"
#include "routine.h"

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if USE_MALLOC_H
#include <malloc.h>
#else
#if NEED_MALLOC_EXTERN
extern char *malloc(), *calloc(), *realloc();
#endif
#endif


/* this code should output a man page template				(ksb)
 * find out the ``common'' name for this page, default ``prog''
 * output everything we know about the options that matters --
 * this is just a template so the user can fix it (delete stuff) later.
 */
void
mkman(fp)
FILE *fp;
{
	extern char *getenv();
	register OPTION *pOR, *pORAlias;
	register int chLast;
	register char *pchForbid;
	register char *pcEnv;
	auto int found;
	auto char *pchUser;
	auto char *pcProg = "prog", *pcMem;

	/* if we have a list of basenames search for the
	 * one that forces no options (the ``common'' one)
	 * if that fails take the first one -- I guess
	 * we *could* take the shortest string?
	 */
	if ((char **)0 != ppcBases) {
		register int i;
		register char *pcOpt;

		pcProg = nil;
		for (i = 0; (char *)0 != ppcBases[i]; ++i) {
			pcOpt = ppcBases[i] + 1 + strlen(ppcBases[i]);
			if ('\000' == *pcOpt) {
				pcProg = ppcBases[i];
				break;
			}
		}
		if (nil == pcProg)
			pcProg = ppcBases[0];
	}
	if (nil == (pchUser = getenv("NAME"))) {
		pchUser = "<your name here>";
	}
	fprintf(fp, ".\\\" %cId%c\n.\\\" by %s\n", '$', '$', pchUser);

	/* produce the .TH three part header, have to toupper()
	 * all the letters in the common name
	 */
	if ((char *)0 != (pcMem = malloc(strlen(pcProg)+1))) {
		register char *pcSrc, *pcDest;

		pcDest = pcMem;
		for (pcSrc = pcProg; '\000' != *pcSrc; ++pcSrc) {
			if (islower(*pcSrc))
				*pcDest++ = toupper(*pcSrc);
			else
				*pcDest++ = *pcSrc;
		}
		*pcDest = '\000';
	}
	fprintf(fp, ".TH %s 1L LOCAL\n", nil != pcMem ? pcMem : pcProg);
	if (nil != pcMem) {
		free(pcMem);
	}

	fprintf(fp, ".SH NAME\n%s", pcProg);
	if ((char **)0 != ppcBases) {
		register int i;

		for (i = 0; (char *)0 != ppcBases[i]; ++i) {
			if (0 == strcmp(pcProg, ppcBases[i])) {
				continue;
			}
			fprintf(fp, ", %s", ppcBases[i]);
		}
	}
	fputs(" \\- undocumented program\n", fp);

	/* The mkuline routines use `\*(PN' for the common name
	 * set it up for them.
	 */
	fputs(".SH SYNOPSIS\n", fp);
	fprintf(fp, ".ds PN \"%s\n", pcProg);
	(void)mkuvector(fp, 1);

	fputs(".SH DESCRIPTION\n", fp);
	/* loop through params and output descriptions for them
	 */
	for (pOR = pORDecl; nilOR != pOR; pOR = pOR->pORnext) {
		if (!ISPPARAM(pOR)) {
			if (ISVISIBLE(pOR)) {
				fprintf(fp, ".\\\" explain %s?\n", pOR->pchname);
			}
			continue;
		}
		if (DIDOPTIONAL(pOR)) {
			fprintf(fp, ".\\\" %s was forced optional\n", pOR->pchdesc);
		}
		fprintf(fp, "The %s positional parameter \\fI%s\\fP", pOR->pOTtype->pchlabel, pOR->pchdesc);
		if (DIDINOPT(pOR)) {
			switch (ISWHATINIT(pOR)) {
			case OPT_INITCONST:
				fprintf(fp, "\n(default value of %s)", pOR->pchinit);
				break;
			case OPT_INITENV:
				fprintf(fp, "\n(default value taken from the environment variable \\fB%s\\fP, if set)", pOR->pchinit);
				break;
			case OPT_INITEXPR:
				fprintf(fp, "\n(default value computed by executing (%s))", pOR->pchinit);
				break;
			}
		}
		if (nil != pOR->pchverb) {
			fprintf(fp, "\n%s", pOR->pchverb);
		}
		fputs(".\n", fp);
	}

	fputs(".SH OPTIONS\n", fp);
	if ((char *)0 != pchGetenv) {
		fprintf(fp, "This command reads the environment variable \\fB%s\\fP for options.\n", pchGetenv);
	}
	if ((char **)0 != ppcBases) {
		register int i;
		register char *pcOpt;

		/* output a line about each basename that makes a
		 * difference (forces options or keeps the default
		 * force from acting)
		 */
		for (i = 0; (char *)0 != ppcBases[i]; ++i) {
			pcOpt = ppcBases[i] + 1 + strlen(ppcBases[i]);
			if ('\000' != *pcOpt) {
				fprintf(fp, "If the program is called as \\fI%s\\fP then ``%s'' is forced.\n", ppcBases[i], pcOpt);
			} else if ((char *)0 != ppcBases[1] || nil == pcDefBaseOpt) {
				fprintf(fp, "If the program is called as \\fI%s\\fP then no options are forced.\n", ppcBases[i]);
			}
		}
		if ((char *)0 != pcDefBaseOpt) {
			fprintf(fp, "If the program is called by any unrecognized name then ``%s'' is forced.\n", pcDefBaseOpt);
		}
	} else if ((char *)0 != pcDefBaseOpt) {
		fprintf(fp, "The program recognizes no common name, thus it forces ``%s'' for each invocation.\n", pcDefBaseOpt);
	}
	for (pOR = pORRoot; nilOR != pOR; pOR = pOR->pORnext) {
		register char *pcHdr;
		if (! ISVISIBLE(pOR) || ISALIAS(pOR))
			continue;

		pcHdr = ".TP\n";
		for (pORAlias = pOR; nilOR != pORAlias; pORAlias = pORAlias->pORalias) {
			if (! ISVISIBLE(pORAlias)) {
				continue;
			}
			fprintf(fp, "%s", pcHdr);
			pcHdr = " | ";
			if (ISSACT(pORAlias)) {
				fprintf(fp, "\\fB%s\\fP", sactstr(pORAlias->chname));
			} else if (ISPPARAM(pORAlias)) {
				fprintf(fp, "\\fI%s\\fP", pORAlias->pchdesc);
			} else if (ISVARIABLE(pORAlias)) {
				fprintf(fp, "\\fI%s\\fP", pORAlias->pchname);
			} else if ('+' == pORAlias->chname) {
				fprintf(fp, "\\fB%s\\fP\\fI%s\\fP", pORAlias->pchgen, pORAlias->pchdesc);
			} else if ('#' == pORAlias->chname) {
				fprintf(fp, "\\fB\\-\\fI%s\\fP", (char *)0 != pORAlias->pchdesc ? pORAlias->pchdesc : "number");
			} else if ('*' == pORAlias->chname) {
				fprintf(fp, "\\fBnoparameter\\fP");
			} else if ('?' == pORAlias->chname) {
				fprintf(fp, "\\fBbadoption\\fP");
			} else if (':' == pORAlias->chname) {
				fprintf(fp, "\\fBotherwise\\fP");
			} else if (sbBInit == pORAlias->pOTtype->pchdef) {
				fprintf(fp, "\\fB\\-%c\\fP", pORAlias->chname);
			} else if (DIDOPTIONAL(pORAlias)) {
				fprintf(fp, "\\fB\\-%c\\fP\\fI%s\\fP", pORAlias->chname, pORAlias->pchdesc);
			} else {
				fprintf(fp, "\\fB\\-%c\\fP \\fI%s\\fP", pORAlias->chname, pORAlias->pchdesc);
			}
		}
		fputc('\n', fp);
		if (nil != pOR->pchverb) {
			Emit(fp, pOR->pchverb, pOR, "\\\n", 0);
			fputc('\n', fp);
		}
		if (ISONLY(pOR)) {
			fputs("This option may only be given once.\n", fp);
		}
		if (ISSTRICT(pOR)) {
			fputs("This option is forbidden by the", fp);
			found = 0;
			chLast = '\000';
			for (pORAlias = pORRoot; nilOR != pORAlias; pORAlias = pORAlias->pORnext) {
				if (!ISFORBID(pORAlias) || nil == strchr(pORAlias->pchforbid, pOR->chname)) {
					continue;
				}
				if ('\000' != chLast) {
					if (found > 1)
						fputc(',', fp);
					fprintf(fp, " \\fB\\-%c\\fP", chLast);
				}
				chLast = pORAlias->chname;
				++found;
			}
			switch (found) {
#if defined(DEBUG)
			case 0:
				fprintf(stderr, "%s: internal error.\n", progname);
				exit(MER_INV);
#endif
			case 1:
				fprintf(fp, " \\fB\\-%c\\fP option", chLast);
				break;
			default:
				fprintf(fp, " and \\fB\\-%c\\fP options", chLast);
				break;
			}
			fputs(".\n", fp);
		}
		if (ISFORBID(pOR) && !( pOR->chname == pOR->pchforbid[0] && '\000' == pOR->pchforbid[1])) {
			fputs("This option forbids the", fp);
			found = 0;
			chLast = '\000';
			for (pchForbid = pOR->pchforbid; *pchForbid; ++pchForbid) {
				if ('\000' != chLast) {
					if (found > 1)
						fputc(',', fp);
					fprintf(fp, " \\fB\\-%c\\fP", chLast);
				}
				chLast = *pchForbid;
				++found;
			}
			switch (found) {
#if defined(DEBUG)
			case 0:
				fprintf(stderr, "%s: internal error.\n", progname);
				exit(MER_INV);
#endif
			case 1:
				fprintf(fp, " \\fB\\-%c\\fP option", chLast);
				break;
			default:
				fprintf(fp, " and \\fB\\-%c\\fP options", chLast);
				break;
			}
			fputs(".\n", fp);
		}
		if ((char *)0 !=  pOR->pOTtype->pchmannote) {
			Emit(fp, pOR->pOTtype->pchmannote, pOR, "\\\n", 0);
		}
	}
	fputs(".SH EXAMPLES\n", fp);
	RLineMan(fp);
	fputs(".SH BUGS\n.SH AUTHOR\n", fp);
	fprintf(fp, "%s\n", pchUser);
	if (nil != (pcEnv = getenv("ORGANIZATION"))) {
		fprintf(fp, ".br\n%s\n", pcEnv);
	}
	if (nil != (pcEnv = getenv("LOGNAME")) || nil != (pcEnv = getenv("USER"))) {
		fprintf(fp, ".br\n%s\n", pcEnv);
	}
	fputs(".SH \"SEE ALSO\"\n", fp);
	fputs("sh(1)\n", fp);
}
