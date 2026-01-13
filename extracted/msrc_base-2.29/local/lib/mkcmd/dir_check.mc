/* $Id: dir_check.mc,v 8.3 2004/12/15 22:20:18 ksb Exp $
 * code to check if a filename is a directory				(ksb)
 */
int
IsDir(pcOpt, pcValue)
char *pcOpt, *pcValue;
{
	auto struct stat stValue;

	if (-1 == stat(pcValue, & stValue)) {
%		fprintf(stderr, "%%s: %%s: %%s: %%s\n", %b, pcOpt, pcValue, %E)%;
		return 0;
	}
	if (S_IFDIR == (stValue.st_mode & S_IFMT)) {
		return 1;
	}
%	fprintf(stderr, "%%s: %%s: %%s: Not a directory\n", %b, pcOpt, pcValue)%;
	return 0;
}
