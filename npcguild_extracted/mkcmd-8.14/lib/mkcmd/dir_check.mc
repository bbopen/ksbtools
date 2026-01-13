/* $Id: dir_check.mc,v 1.1 1997/08/22 02:42:36 ksb Beta $
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
