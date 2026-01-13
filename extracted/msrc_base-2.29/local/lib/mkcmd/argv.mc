/* $Id: argv.mc,v 8.10 2002/12/02 18:05:33 ksb Exp $
 * ksb's support for a list of words taken from mulitple options	(ksb)
 * which together form an argv (kinda).
 */

#if !defined(U_ARGV_BLOB)
#define U_ARGV_BLOB	16
#endif
#if 0 == U_ARGV_BLOB
#undef U_ARGV_BLOB
#define U_ARGV_BLOB	2
#endif

/* Add an element to the present vector, yeah we are N^2,		(ksb)
 * and we depend on the init below putting an initial (char *)0 in slot 0.
 */
static void
u_argv_add(pPPM, pcWord)
struct PPMnode *pPPM;
char *pcWord;
{
	register char **ppc;
	register int i;
	if ((struct PPMnode *)0 == pPPM || (char *)0 == pcWord)
		return;
	ppc = util_ppm_size(pPPM, U_ARGV_BLOB);
	for (i = 0; (char *)0 != ppc[i]; ++i)
		/* nada */;
	ppc = util_ppm_size(pPPM, ((i+1)/U_ARGV_BLOB)*U_ARGV_BLOB);
	ppc[i] = pcWord;
	ppc[i+1] = (char *)0;
}

/* Setup to process all the argv's the Implementor declared.		(ksb)
 * We allocate them all at once, then the initial vectors for each.
 */
static void
u_argv_init()
{
	register struct PPMnode *pPPMInit;
	register char **u_ppc;

%	if ((struct PPMnode *)0 == (pPPMInit = (struct PPMnode *)calloc(%K<u_argv_init>j, sizeof(struct PPMnode)))) %{
%		fprintf(stderr, "%%s: calloc: %%d: no core\n", %b, %K<u_argv_init>j)%;
		exit(60);
	}
%@foreach "u_argv_init"
%	%n = util_ppm_init(pPPMInit++, sizeof(char *), U_ARGV_BLOB)%;
%	u_ppc = (char **)util_ppm_size(%n, 2)%;
	u_ppc[0] = (char *)0;	/* sentinal value, do not remove */
%@endfor
}
