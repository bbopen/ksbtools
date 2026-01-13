#!mkcmd
# $Id: sbp.m,v 1.18 2000/01/30 23:55:01 ksb Exp $
# command line interface for sync-backup-partitions			(ksb)
from '"machine.h"'
from '"read.h"'
from '"sbp.h"'
from '<sys/types.h>'
from '<sys/stat.h>'

require "std_help.m" "std_version.m" "std_control.m"
#require "util_path.m"
require "sbp.mi" "sbp.mh"

basename "sbp" ""
basename "sync-backup-partitions" ""

%i
static char rcsid[] =
	"$Id: sbp.m,v 1.18 2000/01/30 23:55:01 ksb Exp $";

static SBP_ENTRY *pBPBackups;
%%

augment key "PATH" 1 init {
	"/usr/local/bin" "/usr/local/etc"
	"/etc" "/usr/lib/fs/ufs" "/sbin" "/usr/sbin" "/usr/etc" "/stand"
	"/usr/bin" "/bin"
}
key "sbp_dump" 1 init {
	"ufsdump" "dump" "backup" "rdump"
}
key "sbp_restore" 1 init {
	"ufsrestore" "restore" "rrestore"
}
key "sbp_install" 1 init {
	"install"
}
key "sbp_fstab" 1 init {
	"fstab" "vfstab" "checklist" "filesystems"
}

augment action 'V' {
	user 'Version();'
}

boolean 'a' {
	named "fAsync"
	help "mount the destination file systems async to speed the backup"
}
boolean 'i' {
	hidden
	named "fInteract"
	help "be interactive at each major step"
}
boolean 'J' {
	hidden named "fInsane"
	help "debug inode guess code"
}
boolean 'j' {
	named "fJudge"
	help "just guess at the newfs tune for backup partitions"
}
char* 'f' {
	named "pcFstab"
	init "acFstabPath"
	param "fstab"
	help "the file system table to read"
}

char* 'm' {
	named "pcMtPoint"
	init '"/backup"'
	param "backup"
	user "CheckPoint(%n);"
	help "construct the backup partition on this mount-point"
}
char* 'r' {
	named "pcMtRoot"
	init '"/"'
	param "source"
	help "the data source heirarchy"
}
char* 'B' {
	hidden named "pcBefore"
	param "before"
	help "execute before each filesystem is sync'd"
}
char* 'C' {
	named "pcCleanup"
	param "cmd"
	help "cleanup command executed after copy, before umount"
}
char* 'A' {
	hidden named "pcAfter"
	param "after"
	help "execute after each filesystem is sync'd"
}
char* 'H' {
	named "pcHow"
	init '"dump"'
	param "how"
	once user "SetHow(%n);"
	help "specify how to sync the partitions (use ? for help)"
}
double 'D' {
	hidden named "dBlockFull" init "90.0"
	help "the percent of data we expect to use on the file system"
}
double 'I' {
	hidden named "dFileFull" init "60.0"
	help "the percent of inode that should be used when we are at capacity"
}

after {
	named "ReadFs"
	update "pBPBackups = %n(pcFstab);"
}

list {
	named "DoSync"
	param "file-systems"
	update "%n(%#, %@, pBPBackups);"
	help "list of partitions to sync"
}

%c
/* explain what we found						(ksb)
 */
static void
Version()
{
	printf("%s: backup mount point: %s\n", progname, pcMtPoint);
	printf("%s: list: %s\n", progname, acFstabPath);
	printf("%s: dump: %s\n", progname, acDumpPath);
	printf("%s: restore: %s\n", progname, acRestorePath);
	printf("%s: install: %s\n", progname, acInstallPath);
}

/* a stron spell like this must have some safe-guards			(ksb)
 * the proposed backup mount-point must
 *	start with a slash
 *	not be "/", "/usr", "/opt", "/var" (or /home?)
 */
static void
CheckPoint(pcParam)
char *pcParam;
{
	if ('/' != pcParam[0]) {
		fprintf(stderr, "%s: backup mount point \"%s\" must start with a slash\n", progname, pcParam);
		exit(2);
	}
	switch (pcParam[1]) {
	case '\000':
		break;
	case 'u':
		if (0 == strcmp("/usr", pcParam)) {
			break;
		}
		if (0 == strcmp("/usr/local", pcParam)) {
			break;
		}
		return;
	case 'v':
		if (0 == strcmp("/var", pcParam)) {
			break;
		}
		return;
	case 'o':
		if (0 == strcmp("/opt", pcParam)) {
			break;
		}
	default:
		return;
	}
	fprintf(stderr, "%s: a backup mount of \"%s\" would destroy the system\n", progname, pcParam);
	exit(2);
}
%%
