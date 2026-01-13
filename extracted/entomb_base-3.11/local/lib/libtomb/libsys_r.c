/* emulate creat(2) with syscall					(ksb)
 */
int
creat_r(pcName, iMode)
char *pcName;
int iMode;
{
	return creat(pcName, iMode);
}

/* use syscall to emulate the real open routine				(ksb)
 */
int
open_r(file, flags, iMode)
char *file;
int flags, iMode;
{
	return open(file, flags, iMode);
}

/* emulate rename(2) with syscall(2)					(ksb)
 */
int
rename_r(from, to)
char *from, *to;
{
	return rename(from, to);
}

/* emulate truncate(2) with syscall(2)					(ksb)
 */
int
truncate_r(pcPath, iLength)
char *pcPath;
int iLength;
{
	return truncate(pcPath, iLength);
}

/* emulate unlink for calls to the Real Thing				(ksb)
 */
int
unlink_r(pcName)
char *pcName;
{
	return unlink(pcName);
}
