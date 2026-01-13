#!/usr/bin/perl -T
# An example perl jacket/helmet script (parses the options for you).	(ksb)
# Note that this code will most be run under the taint rules, see perlsec(1).
# KS Braunsdorf, at the NPCGuild.org
# $Doc: sed -e 's/&/A''MPamp;/g' -e 's/</\\&lt;/g' -e 's/>/\\&gt;/g' <%f| sed -e 's/[A][M][P]/\\&/g'

use lib  '/usr/local/lib/sac/perl'.join('.', unpack('c*', $^V)),
	'/usr/local/lib/sac';
use Getopt::Std;
use strict;

my($hisPath) = $ENV{'PATH'};
$ENV{'PATH'} = '/usr/bin:/bin:/usr/local/bin:/usr/local/sbin:/sbin';
my($progname, %opts, $usage);
$progname = $0;
$progname =~ s/.*\///;
getopts("VhP:u:g:f:R:C:M:", \%opts);
$usage = "$progname: usage [-P pid] [-u user] [-g group] [-f file] [-R root] [-M ptm] -C config -- mnemonic program euid:egid cred_type:cred";

if ($opts{'V'}) {
	print "$progname: ", '$Id: jacket.pl,v 2.47 2010/03/18 20:38:11 ksb Exp $', "\n";
	exit 0;
}
if ($opts{'h'}) {
	print "$usage\n",
		"C config   which op configuration file sourced the rule\n",
		"f file     the file specification given to op, as an absolute path\n",
		"g group    the group specification given to op\n",
		"h          standard help output\n",
		"M ptm      manage process input/output on this ptm descriptor\n",
		"P pid      the process-id of the jacketed process (only as a jacket)\n",
		"R root     the directory we chrooted under\n",
		"u user     the user specification given to op\n",
		"V          standard version output\n",
		"mnemonic   the requested mnemonic\n",
		"program    the program mapped from the mnemonic\n",
		"euid:egid  the computed effective uid and gid\n",
		"cred_type  the credential type that granted access (groups, users, or netgroups)\n",
		"cred       the matching group, login, or netgroup\n";
	exit 0;
}

my($MNEMONIC, $PROGRAM);
shift @ARGV if ('--' eq $ARGV[0]);
if (scalar(@ARGV) != 4) {
	print STDERR "$progname: exactly 4 positional parameters required\n";
	print "64\n" if $opts{'P'};
	exit 64;
}
if ($ARGV[0] !~ m|^([-/\@\w.]+)$|o) {
	print STDERR "$progname: mnemonic is zero width, or spelled badly\n";
	print "64\n" if $opts{'P'};
	exit 64;
}
$MNEMONIC = $1;
if ($ARGV[1] !~ m|^([-/\@\w.]+)$|o) {
	print STDERR "$progname: program specification looks bogus\n";
	print "64\n" if $opts{'P'};
	exit 64;
}
$PROGRAM = $1;
if ($ARGV[2] !~ m/^([^:]*):([^:]*)$/o) {
	print STDERR "$progname: euid:egid $ARGV[2] missing colon\n";
	print "65\n" if $opts{'P'};
	exit 65;
}
my($EUID, $EGID) = ($1, $2);
if ($ARGV[3] !~ m/^([^:]*):([^:]*)$/o) {
	print STDERR "$progname: cred_type:cred $ARGV[3] missing colon\n";
	print "76\n" if $opts{'P'};
	exit 76;
}
my($CRED_TYPE, $CRED) = ($1, $2);

# Now $MNEMONIC is mnemonic, $PROGRAM is program, also $EUID, $EGID,
# $CRED_TYPE, $CRED are set -- so make your checks now.
#
# There are 5 actions you can take, and leading white-space is ignored:
# 1) As above you can output an exit code to the process:
#	print "120\n";
# 2) You can set an environment variable [be sure to backslash the dollar]:
#	print "\$FOO=bar\n"
#    The same line without a value adds the client's $FOO (as presented):
#	print "\$FOO\n";
# 3) You can remove any environment variable:
#	print "-FOO\n";
# 4) You can send a comment which op will output only if -DDEBUG was set
#    when op was built [to help you, Mrs. Admin]:
#	print "# debug comment\n";
# 5) Use op to signal your displeasure with words, making op prefix your
#    comment with "op: jacket: " ("op: helmet: "):
#	print "Permission lost!\n";
#    (This suggests an exit code of EX_PROTOCOL.)
#
# Put your checks and payload here.  Output any commands to the co-process,
# be sure to send a non-zero exit code if you want to stop the access!
# CHECKS AND REPARATIONS
#e.g. check LDAP, kerberos, RADIUS, or time-of-day limits here.

# If we are a helmet you can just exit, if you exit non-zero op will view that
# as a failure to complete the access check, so it won't allow the access.
exit 0 unless $opts{'P'};

# We must be a jacket, and the requested access is not yet running.
# You could set a timer here, or capture the start/stop times etc.
# CAPTURE START DATA
#e.g. call time or set an interval timer
#e.g. block signals
my($PTM) = undef;
if (defined($opts{'M'})) {
	# This descriptor number is opened to the master side
	# of the inferrior process.  Setup anything you need here.
	# make an IO handle from the fd
	open $PTM, "+<&=$opts{'M'}" or
		die "fdopen: $opts{'M'}: $!\n";
}

# Let the new process continue by closing stdout, if the last exitcode
# you wrote to stdout was non-zero op won't run the command, I promise.
open STDOUT, ">/dev/null";

# We can wait for the process to exit, we are in perl because the shell
# (ksh,sh) can't get the exit code from a process it didn't start.
my($kid, $status);
$kid = waitpid $opts{'P'}, 0;
$status = $? >> 8;

# if $PTM is open you can read/write data to the process.

# Do any cleanup you want to do here, after that the jacket's task is complete.
# Mail a report, syslog something special, restart a process you stopped
# before the rule ran, what ever you need.  On a failure you should exit
# with a non-zero code here.
# CLEANUP
#e.g.: print STDERR "$kid exited with $status\n";
#e.g.: use Sys::Syslog; ... log something clever

# This is the exit code that goes back to the client, since this jacket
# became the op process (as all jackets do).
exit 0;
