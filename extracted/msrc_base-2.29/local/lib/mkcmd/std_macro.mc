/* $Id: std_macro.mc,v 8.9 2008/08/16 21:19:02 ksb Exp $
 */

/* list the macros in the table to the file 				(ksb)
 */
void
ListMacros(fp)
FILE *fp;
{
	register int iWidth;
	register struct MCnode *u_pMC;

	iWidth = 1;
%	for (u_pMC = %roZK1v%; (char *)0 != u_pMC->u_pcname%; ++u_pMC) %{
		register int l;
		if (iWidth < (l = strlen(u_pMC->u_pcname)))
			iWidth = l;
	}
	iWidth *= -1;
%	for (u_pMC = %roZK1v%; (char *)0 != u_pMC->u_pcname%; ++u_pMC) %{
%		fprintf(fp, "%%s: %%*s %%s\n", %b, iWidth, u_pMC->u_pcname, (char *)0 != u_pMC->u_pccur ? u_pMC->u_pccur : %roZK6v)%;
	}
}

/* search the macro table for a macro by name				(ksb)
 */
static struct MCnode *
FindMacro(pcMacro)
char *pcMacro;
{
	register struct MCnode *u_pMC;

%	for (u_pMC = %roZK1v%; (char *)0 != u_pMC->u_pcname%; ++u_pMC) %{
		if (0 == strcmp(pcMacro, u_pMC->u_pcname)) {
			return u_pMC;
		}
	}
	return (struct MCnode *)0;
}

/* return the value of a macro or					(ksb)
% * %roZK3v for failure
 */
char *
Macro(pcMacro)
char *pcMacro;
{
	register struct MCnode *pMC;

	if ((struct MCnode *)0 == (pMC = FindMacro(pcMacro))) {
%		return %roZK3v%;
	}
	return pMC->u_pccur;
}

/* set a macro								(ksb)
 */
static void
DoMacro(pcMacro)
char *pcMacro;
{
	register char *pcEq;
	register struct MCnode *pMC;

	if ((char *)0 != (pcEq = strchr(pcMacro, '='))) {
		*pcEq = '\000';
	}
	if ((struct MCnode *)0 == (pMC = FindMacro(pcMacro))) {
%		%XK<macro_hooks>2v
	}
	if ((char *)0 != pcEq) {
		*pcEq++ = '=';
		pMC->u_pccur = pcEq;
	} else {
		pMC->u_pccur = pMC->u_pcon;
	}
}

/* reset a macro							(ksb)
 */
static void
UndoMacro(pcMacro)
char *pcMacro;
{
	register struct MCnode *pMC;

	if ((struct MCnode *)0 == (pMC = FindMacro(pcMacro))) {
%		%XK<macro_hooks>2v
	}
	pMC->u_pccur = pMC->u_pcoff;
}
