/*@Explode@*/
/* $Id: util_fts.mc,v 8.3 2004/12/15 22:20:18 ksb Exp $
 */

/* example fts_walk routine for the fts interface			(ksb)
 * by default scan key "fts_path" (if no nodes were presented)
 */
void
u_fts_walk(argv, fts_opts)
char **argv;
int fts_opts;
{
	register FTS *u_ftsp;
	register FTSENT *p;
	extern int errno;

	if ((char **)0 == argv || (char *)0 == *argv) {
%		static char *apcDef[] = %{ %K<fts_def_path>/ql, (char *)0%}%;
		argv = apcDef;
	}
	if ((FTS *)0 == (u_ftsp = fts_open(argv, fts_opts, (int (*)())0))) {
%		fprintf(stderr, "%%s: fts_open: %%s\n", %b, %E)%;
		exit(errno);
	}
	while ((FTSENT *)0 != (p = fts_read(u_ftsp))) {
%		%H/u_ftsp/H/p/K<fts_visit>/ef%;
	}
	(void)fts_close(u_ftsp);
}
