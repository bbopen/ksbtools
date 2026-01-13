#!/usr/local/bin/perl
# $Id: Installus.pl,v 2.2 2000/06/21 17:23:34 ksb Exp $
#
# Installus.pl
# A script that calls "installus" recursively on a large directory
# structure.
#
# TODO: Should have some way to remember the password for me so
# I don't have to type it at the keyboard. -- ksb

$program = "Installus.pl";
$rcs_id = '$Id: Installus.pl,v 2.2 2000/06/21 17:23:34 ksb Exp $';

# $Log: Installus.pl,v $
# Revision 2.2  2000/06/21 17:23:34  ksb
# bugs fixed
#
# Revision 2.1  1999/08/09  13:32:23  ksb
# made usage output sane
#
# Revision 2.0  1999/08/09  13:24:46  ksb
# major upgrade from 1.X, reverse -V and -v to match other ksb tools
#
# Revision 1.11  1998/02/02 15:32:57  james
# Fixed up the -i support code.
#
# Revision 1.10  1998/01/13 15:11:37  james
# Now supporting the -i option.
#
# Revision 1.9  1997/09/23 14:16:15  james
# Doing some select magic so that output is
# not duplicated by parent & child.
# Apparently installus exits non-zero a lot, so
# we no longer have some error handling code.
#
# Revision 1.8  1997/09/21 13:44:42  james
# Fixed the installus of files,
# stopped forking when creating a directory,
# because future installus commands will require that directory
# to exist.
#
# Revision 1.7  1997/09/21 13:27:55  james
# Added -P command line option
#
# Revision 1.6  1997/09/08 21:09:03  james
# Fixed a big boo-boo, where no command was ever executed.
# Whoops.
#
# Revision 1.5  1997/09/08 20:56:53  james
# Fixed a bug in creating directories
#
# Revision 1.4  1997/09/08 20:51:33  james
# Works good, I think.  The first good version.  Giddyup.
#
# Revision 1.3  1997/09/08 20:25:53  james
# Tried to fix rcs_id
#
# Revision 1.2  1997/09/08 20:24:40  james
# Fixed rcs_id
#
# Revision 1.1  1997/09/08 20:24:04  james
# Initial revision
#

# tune interfaces to other programs
$installus = "/usr/local/etc/installus";
$file_opts = "-c";
$dir_opts = "-dr";

# our interface
use Getopt::Std;
use POSIX;

Getopt::Std::getopts('nvVhiP:');
if ($opt_V) {
    &version();
    exit(0);
}
if ($opt_h) {
    &usage();
    exit(0);
}
if ($opt_P) {
    if ($opt_P == 0) {
        &usage();
        exit(1);
    }
} else {
    $opt_P = 1;
}

@dirs = ();
while ($_ = shift) {
    if (-f $_ || -d $_) {
	# nothing special
    } elsif (-e $_)  {
	print STDERR "$program: $_: source must be a file or a directory\n";
	exit(1);
    }
    push(@dirs, $_);
}
$dest = pop(@dirs);

if ($dest eq "") {
    print STDERR "$program: missing destination\n";
    &usage();
    exit(1);
}
if (-f $dest) {
    print STDERR "$program: $dest is a file, not a directory\n";
    exit(1);
}

# setup so fork'd copies don't duplicate output
select STDERR;
$| = 1;
select STDOUT;
$| = 1;

# Install all the files we were asked to installus		(jkoh, ksb)
# XXX: we should perform more tests -- robust=good!
if (!(-e $dest)) {
    $cmd = "$installus $dir_opts $dest";
    if ($opt_v) { print $cmd . "\n"; }
    if (!$opt_n) { `$cmd` || die "Cannot make destination directory"; }
}
while ($f = pop(@dirs)) {
    if (-f $f) {
        $f =~ /(^.*)\/[^\/]*/;
        $subdir = $1;
	if (!(-d "$subdir")) {
	    $cmd = "$installus $dir_opts $subdir";
	    if ($opt_v) { print $cmd . "\n"; }
	    if (!$opt_n) { `$cmd`; }
	}
	$cmd = "$installus $file_opts $f $dest/$subdir/.";
        if ($opt_i) {
	    $cksum1 = "jkoh";
            if (-r $f) {
                `cksum $f` =~ /^([\d\s]*)/;
                $cksum1 = $1;
            }
	    $cksum2 = "ksb";
            if (-r "$dest/$f") {
                `cksum $dest/$f` =~ /^([\d\s]*)/;
                $cksum2 = $1;
            }
	    if ($cksum1 eq $cksum2) {
		next;
	    }
	}
	if ($opt_v) { print $cmd . "\n"; }
	if (!$opt_n) { &forkthis($cmd); }
	next;
    }
    # must be a directory (-d was true in args check)
    # skip and OLD directories in the source tree
    if ($f =~ /^.*\/OLD\$/) {
	if ($opt_v) { print "skip OLD directory $f\n"; }
	next;
    }
    if (!(-e "$dest/$f")) {
	$cmd = "$installus $dir_opts $dest/$f";
	if ($opt_v) { print $cmd . "\n"; }
	if (!$opt_n) { `$cmd`; }
    }
    foreach $sub (<$f/*>) {
	push(@dirs, $sub);
    }
}
exit(0);

# subroutines:

# keep up to $opt_P jobs running					(jkoh)
@pids = ();
sub forkthis {
    $cmd = shift;
    if (@pids >= $opt_P) {
	($child, @other) = @pids;
	@pids = @other;
	waitpid($child, 0);
#        if ($? != 0) { 
#            print STDERR "$program: exiting with $?\n";
#            exit $?;
#        }
    }
  FORK: {
      if ($child = fork) {
	  # parent
	  @new = (@pids, $child);
	  @pids = @new;
      } elsif (defined $child) {
	  # child
	  `$cmd`;
	  exit 0;
      } elsif ($! =~ /No more process/) {
	  sleep 5;
	  redo FORK;
      } else {
	  die "can't fork: $!\n";
      }
  }
}

# output our release information					(jkoh)
sub version {
    print $rcs_id . "\n";
    return 0;
}

# output our usage information						(jkoh)
sub usage {
    print "$program: usage [-vni] [-P jobs] srcfiles destdir\n";
    print "$program: usage -h\n";
    print "$program: usage -V\n";
    print "h       display this message and exit\n";
    print "i       do an incremental install (only changed files)\n";
    print "n       don't really execute the commands\n";
    print "P jobs  run 'jobs' number of jobs in parallel\n";
    print "v       verbose mode\n";
    print "V       show version number and exit\n";
    print "srcfile relative path to source hierarchy\n";
    print "destdir a destination directory under installus control\n";
    print "\nFor example: $program foo.html bar/ /opt/www\n";
    print " installs /opt/www/foo.html, /opt/www/bar/...\n";
    return 0;
}
