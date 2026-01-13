/* $Id: glob_ieee.mc,v 8.4 1997/10/05 20:55:08 ksb Beta $
 * glob with the 4.4 BSD glob interface, fins and extra honey please! (ksb)
 */

static glob_t u_ieee_gbuf;	/* private glob buffer */

/* return a vector of matched files					(ksb)
 * using IEEE glob() as the backend
 */
char **
%%K<glob_functions>1ev(pcDo)
char *pcDo;
{
	u_ieee_gbuf.gl_offs = 2;
%	if (0 != glob(pcDo, %K<glob_options>/i!|!1.2-$<cat>, %K<glob_functions>3ev, & u_ieee_gbuf)) %{
		return (char **)0;
	}
	return u_ieee_gbuf.gl_pathv;
}

/* free all that core the glob code used				(ksb)
 */
void
%%K<glob_functions>2ev(ppcGlob)
char **ppcGlob;
{
	if ((char **)0 != ppcGlob && ppcGlob == u_ieee_gbuf.gl_pathv) {
		globfree(& u_ieee_gbuf);
	}
}
