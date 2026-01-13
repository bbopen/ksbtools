#!/usr/bin/perl
# $Id: owners2instck.pl,v 1.5 2008/10/30 22:47:44 anderson Exp $
#

use lib '/usr/local/lib/sac/perl'.join('.', unpack('c*', $^V)),
	'/usr/local/lib/sac';

use Getopt::Std;
use strict;

# convert 555 -> -r-xr-xr-x					(ksb,petef)
sub modetoascii {
	my($mode) = shift;
	my(@ascii) = ('-', 'r', 'w', 'x','r', 'w', 'x','r', 'w', 'x');
	my($i, $j);

	for($i = 0400, $j = 1; $i > 0; $i >>= 1, $j++) {
		if (0 == ($i & $mode)) {
			$ascii[$j] = '-';
		}
	}

	if ($mode & 01000) {
		if ($ascii[9] eq '-') {
			$ascii[9] = 'T';
		} else {
			$ascii[9] = 't';
		}
	}

	if ($mode & 02000) {
		if ($ascii[6] eq '-') {
			$ascii[6] = 'S';
		} else {
			$ascii[6] = 's';
		}
	}

	if ($mode & 04000) {
		if ($ascii[3] eq '-') {
			$ascii[3] = 'S';
		} else {
			$ascii[3] = 's';
		}
	}

	return join('', @ascii);
}

my(%args, $progname);
my($file, $line, $glob, $who, $opts, $mode, $authuser, $authgroup);
my($user, $group, $slink, $hlink, $isdir);

getopts("hV", \%args);
$progname = $0;
$progname =~ s/.*\///;

if (defined($args{'h'})) {
	print "$progname: usage [-hV] file\n";
	print "h	this help message\n";
	print "V	show version information\n";
	print "file	a .owners-format file or \"-\" for stdin\n";
	exit(0);
}

if (defined($args{'V'})) {
	print "$progname: " . '$Id: owners2instck.pl,v 1.5 2008/10/30 22:47:44 anderson Exp $' . "\n";
	exit(0);
}

if (scalar(@ARGV) != 1) {
	print STDERR "$progname: incorrect number of arguments\n";
	exit(1);
}

$file = $ARGV[0];
$file = "<&STDIN" if $file eq '-';

open(OWNERS, "$file");
# we expect lines to be in the standard owners(5L) format:
# glob  user.group  install-opts
while(chomp($line = <OWNERS>)) {
	next if $line =~ /^#/;	# skip comments
	next if $line =~ /^[ 	]*$/; # skip empty lines
	$line =~ m/^([^ 	]*)[ 	]+([^	 ]*)[ 	]+(.*)$/;
	$glob = $1; $who = $2; $opts = $3;
	$authuser = $authgroup = $mode = $slink = $hlink = '';
	$isdir = 0;
	$user = $group = "*";
	($authuser, $authgroup) = split /[.:]/, $who;
	$opts =~ /-m[ 	]*([-0-7drwxtTsSl]+)/ && do {
		$mode = $1;
		$mode =~ s/^0*(.)/$1/;
		$mode = modetoascii(oct $mode) if $mode =~ /^[0-7]+$/;
	};
	$opts =~ /-o[ 	]*([^ 	]+)/ && do {
		$user = $1;
		if ($user =~ /(.+):(.+)/) {
			$user = $1;
			$group = $2;
		}
	};
	$opts =~ /-g[ 	]*([^ 	]*)/ && do {
		$group = $1;
	};
	$opts =~ /-d[ 	]/ && do {
		$isdir = 1;
	};
	$opts =~ /-S[ 	]*([^ 	]*)/ && do {
		$slink = $1;
	};
	$opts =~ /-H[ 	]*([^ 	]*)/ && do {
		$hlink = $1;
	};

	if ($isdir && $mode =~ /^-/) {
		$mode =~ s/^./d/;
	}

	# output instck .cf line
	if ($slink) {
		foreach(split /:/, $slink) {
			printf "%-35s \@%s\n", $_, $glob;
		}
	}
	if ($hlink) {
		foreach(split /:/, $hlink) {
			printf "%-35s :%s\n", $_, $glob;
		}
	}
	printf "%-35s %-10s %-8s %-8s - # %s:%s\n", $glob, $mode, $user, $group, $authuser, $authgroup;
}
close(OWNERS);
