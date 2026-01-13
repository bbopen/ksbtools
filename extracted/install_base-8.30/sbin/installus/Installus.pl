#!/usr/local/bin/perl
# $Id: Installus.pl,v 2.6 2008/10/30 22:47:44 anderson Exp $
#
# Installus.pl
# A script that calls "installus" recursively on a large directory
# structure.
#
# TODO: Should have some way to remember the password for me so
# I don't have to type it at the keyboard. -- ksb

use lib '/usr/local/lib/sac/perl'.join('.', unpack('c*', $^V)),
	'/usr/local/lib/sac';
use strict;
use Getopt::Std;
use File::Basename;
use POSIX;

use vars qw($progname $rcs_id $installus $file_opts $dir_opts
	@ignorelist
	%opts);
$progname = "Installus.pl";
$rcs_id = '$Id: Installus.pl,v 2.6 2008/10/30 22:47:44 anderson Exp $';

# Tune our interfaces to other programs
$installus = "/usr/local/etc/installus";
$file_opts = "-c";
$dir_opts = "-dr";

# Subroutines and globals

use vars qw(@pids);
@pids = ();
sub forkthis {
	my($cmd) = shift;
	my($child);

	if (scalar(@pids) >= $opts{'P'}) {
		$child = shift(@pids);
		waitpid($child, 0);
	}
	while (1) {
		if ($child = fork) {
			# parent
			if ($opts{'d'}) {
				unshift(@pids, $child);
			} else {
				waitpid($child, 0);
			}
			return;
		}
		if (defined $child) {
			# child
			exec $cmd;
			exit 0;
		}
		if ($! =~ m/No more process/o) {
			sleep 5;
			next;
		}
		die "$progname: fork: $!\n";
	}
}

sub version {
	print "$progname: $rcs_id\n";
	print "$progname: installus as \`$installus'\n";
	print "For example: '$progname foo.html bar/ /opt/www' runs:\n";
	print " $installus $file_opts /opt/www/foo.html /opt/www/foo.html\n";
	print " $installus $dir_opts /opt/www/bar /opt/www/bar/\n";
	print " $installus $file_opts /opt/www/bar/1 /opt/www/bar/1\n";
	print " $installus $file_opts /opt/www/bar/2 /opt/www/bar/2 ...\n";
	return 0;
}

sub usage {
	print "$progname: usage [-dinv] [-I file] [-P jobs] srcfiles destdir\n";
	print "$progname: usage -h\n";
	print "$progname: usage -V\n";
	print "d       don't wait on children\n";
	print "h       display this message and exit\n";
	print "i       do an incremental install (only changed files)\n";
	print "I file  ignore all directories and files matching entries in file\n";
	print "n       don't really execute the commands\n";
	print "P jobs  run 'jobs' number of jobs in parallel\n";
	print "v       verbose mode, trace actions and report more details\n";
	print "V       show version number and exit\n";
	print "srcfile relative path to source hierarchy\n";
	print "destdir a destination directory under installus control\n";
	return 0;
}

# main

getopts('nvVhiI:P:d', \%opts);

if ($opts{'V'}) {
	&version();
	exit(0);
}
if ($opts{'h'}) {
	&usage();
	exit(0);
}
if ($opts{'P'}) {
	if ($opts{'P'} == 0) {
		&usage();
		exit(1);
	}
} else {
	$opts{'P'} = 1;
}
if ($opts{'I'}) {
	open(IGNOREFILE, $opts{'I'});
	@ignorelist=<IGNOREFILE>;
	close(IGNOREFILE);
}

select STDERR;
$| = 1;
select STDOUT;
$| = 1;

{
	my($dest, $cmd);
	my(@dirs) = ();

	while ($_ = shift) {
		if (-f $_ || -d $_) {
			# nothing special
			push(@dirs, $_);
			next;
		}
		if (-e $_) {
			print STDERR "$progname: $_: source must be a file or a directory\n";
			exit(1);
		}
		print STDERR "$progname: $_: no such file or directory\n";
		exit(2);
	}

	# sanity check the rest of the line
	$dest = pop(@dirs);
	if ($dest eq "") {
		print STDERR "$progname: no destination, see -h for help\n";
		exit(4);
	}
	if (scalar(@dirs) < 1) {
		print STDERR "$progname: no source files, see -h for help\n";
		exit(5);
	}

	# make destination
	if (-f $dest) {
		print STDERR "$dest is a file\n";
		exit(1);
	}
	if (!(-e $dest)) {
		$cmd = "$installus $dir_opts $dest";
		print $cmd . "\n" if ($opts{'v'});
		if (!$opts{'n'}) {
			`$cmd` || die "$dest: Cannot create";
		}
	}

	# install the fleet
	my($target, $dir, $ignore);
	my($subdir, $entry);
	while ($target = shift(@dirs)) {
		$dir = basename($target);

		if (-f $target) {
			$ignore = grep /^$dir$/, @ignorelist;
			if ($ignore) {
				print "skipping $target\n" if ($opts{'v'});
				next;
			}

			$target =~ m/(^.*)\/[^\/]*/o;
			$subdir = $1;

			if ($opts{'i'}) {
				my($cksumt) = "foo";
				my($cksumd) = "bar";
				if (-r $target) {
					`cksum $target` =~ m/^([\d\s]*)/o;
					$cksumt = $1;
				}
				if (-r "$dest/$target") {
					`cksum $dest/$target` =~ m/^([\d\s]*)/o;
					$cksumd = $1;
				}
				next if ($cksumt eq $cksumd);
			}
			$cmd = "$installus $file_opts '$target' '$dest/$target'";
			print $cmd . "\n" if ($opts{'v'});
			&forkthis($cmd) unless ($opts{'n'});
			next;
		}
		# this had better be a directory (since it is no a file)
		if (! -d $target) {
			print STDERR "$target: neither a file nor a directory\n";
			next;
		}
		# it is, we can push more stuff on to do from that subtree
		next if ($ignore);
		next if ($dir eq "OLD" || $dir eq "CVS");
		if (!(-e "$dest/$target")) {
			$cmd = "$installus $dir_opts $dest/$target";
			print $cmd . "\n" if ($opts{'v'});
			`$cmd` unless ($opts{'n'});
		}

		# If we can open the directory, then include the contents
		# even the file "...", but not "." or "..".
		next unless opendir NEWSTUFF, $target;
		while  ($entry = readdir NEWSTUFF) {
			next if ($entry eq '..' || $entry eq '.');
			push(@dirs, "$target/$entry");
		}
		closedir NEWSTUFF;
	}
}

# exit happy
exit(0);
