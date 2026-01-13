#!/usr/local/bin/perl
# $Id: fixother.pl,v 1.4 2008/10/30 22:47:44 anderson Exp $
#

use lib '/usr/local/lib/sac/perl'.join('.', unpack('c*', $^V)),
	'/usr/local/lib/sac';
use strict;

my($target, $gid, $parentgid, $parentdir, $entry, $group, $progname);
my(@dirs) = ();
my(@grinfo) = ();
my(@stat) = ();
my(%gids, %groups, %dirgids);
my($defgroup) = 'root';		# for unmapped GIDs
my($lastgroup) = ':';
my($filelength) = 0;

$progname = $0;
$progname =~ s/.*\///;

foreach(@ARGV) {
	push(@dirs, $_);
}
if (0 == scalar(@dirs)) {
	push(@dirs, '/');
}

# get a cache of GIDs
open(GROUP, "/etc/group");
while(<GROUP>) {
	chomp($_);
	@grinfo = split /:/;
	if ($gids{$grinfo[2]}) {
		print STDERR "$progname: duplicate GIDs: " . $gids{$grinfo[2]} . " and " . $grinfo[0] . " share " . $grinfo[2] . "\n";
		$filelength++;
	}
	$gids{$grinfo[2]} = $grinfo[0];
	if ($groups{$grinfo[0]}) {
		print STDERR "$progname: duplicate groups: " . $groups{$grinfo[0]} . " and " . $grinfo[2] . " share " . $grinfo[0] . "\n";
		$filelength++;
	}
	$groups{$grinfo[0]} = $grinfo[2];
}
close(GROUP);

die "$progname: not continuing without a valid /etc/group\n" if $filelength;

$dirgids{''} = $groups{'root'};
$dirgids{'/'} = $groups{'root'};

# fix homedirs first
while ($target = shift(@dirs)) {
	$target =~ s|^//*|/|;
	$parentdir = $target;
	$parentdir =~ s|/[^/]*/?$||;
	$parentgid = $dirgids{$parentdir};
	@stat = stat($target);
	$gid = $stat[5];
	if ($gid == $groups{'other'} && 0 == ($stat[2] & 02000) && ($stat[2] & 07) == (($stat[2] & 070) >> 3)) {
		$gid = $parentgid;
		$group = $gids{$gid} ? $gids{$gid} : $defgroup;
		if ($group eq $lastgroup && $filelength <= 16*1024) {
			print " $target";
			$filelength += length($target);
		} else {
			print "\nchgrp " . $group . " " . $target;
			$filelength = length($target);
		}
		$lastgroup = $group;
	}
	next if -f "$target";
	$dirgids{$target} = $gid;

	next if $target =~ m/^\/(net|home|proc)$/;	# don't descend
	next unless opendir NEWSTUFF, $target;
	while  ($entry = readdir NEWSTUFF) {
		next if ($entry eq '..' || $entry eq '.');
		push(@dirs, "$target/$entry");
	}
	closedir NEWSTUFF;
}
print "\n" unless $lastgroup eq ':';

# exit happy
exit(0);
