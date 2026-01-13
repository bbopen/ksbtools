# $Id: HXMD.pm,v 1.10 2009/01/13 20:49:11 anderson Exp $
package HXMD;

use 5.008008;
use strict;
use warnings;
use Data::Dumper;

require Exporter;

our @ISA = qw(Exporter);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

# This allows declaration	use HXMD ':all';
# If you do not need this, moving things directly into @EXPORT or @EXPORT_OK
# will save memory.
our %EXPORT_TAGS = ( 'all' => [ qw(new) ] );
our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );
our @EXPORT = qw( main );
our $VERSION = sprintf "%d.%03d", q$Revision: 1.10 $ =~ /: (\d+)\.(\d+)/;


# Preloaded methods go here.

sub main {

my $token = '';
my $lastc = undef;
my $line = 0;

# Read any hxmd or distrib configuration file into a hashref		(ksb)
sub ReadDB($ $ $ $ $) {
	my($ref,$file,$key,$add,$overwrite) = @_;
	my($tok,$col1,$last,$a,$v);

	return undef unless 'HASH' eq ref($ref);
	$key ||= 'HOST';
	$add ||= 1;
	$overwrite ||= 1;
	my @header = ($key);
	my%binding = ();

	my($curfile);
	return undef unless defined($file);
	return undef unless open $curfile, "<$file";
	$main::line = 1;
	$last = undef;
	$tok = Token($curfile);
	while (defined($tok)) {
		if ('=' eq $tok) {
			warn "$file:$main::line stray operator \"$tok\"\n";
			return undef;
		}
		if ('newline' eq $tok) {
			$tok = Token($curfile);
			next;
		}
		if ('%' eq $tok) {
			foreach my $m (@header) {
				delete $binding{$m};
			}
			@header = ();
			while (defined($tok = Token($curfile))) {
				last if ('newline' eq $tok);

				if ('.' eq $tok) {
					push @header, '.';
					next;
				}
				if ('M' ne $tok) {
					warn "$file:$main::line header contruction must include only m4 macro names, or the undef token (.)\n";
					return undef;
				}
				push @header, $main::token;
				delete $binding{$main::token};
			}
			if ('newline' ne $tok) {
				warn "$file: $main::line header syntax error\n";
				return undef;
			}
			next;
		}

		# We have a definition line or an assignment, find out which.
		$last = $tok;
		if ('.' eq $tok) {
			$col1 = undef;
		} else {
			$col1 = $main::token;
		}
		return undef if (!defined($tok = Token($curfile)));
		if ('=' eq $tok) {
			return undef if (!defined($tok = Token($curfile)));
			if (!defined($col1)) {
				warn "$file: $main::line syntax error: .=$main::token\n";
				return undef;
			} elsif ('W' eq $tok || 'M' eq $tok || '"' eq $tok) {
				$binding{$col1} = $main::token;
			} elsif ('.' eq $tok) {
				delete $binding{$col1};
			} else {
				warn "$file: $main::line $col1= syntax error\n";
				return undef;
			}
			$tok = Token($curfile);
			# should be a newline, don't look for now --ksb
			next;
		}
		# This must be a definition line, assign the column macros.
		foreach $a (@header) {
			if (!defined($last)) {
				warn "$file:$main::line $a: out of data\n";
				return undef;
			}
			if ('.' eq $a) {
				# Allow a header to ignore a column
			} elsif ('.' eq $last) {
				delete $binding{$a};
			} elsif ('M' eq $last || '"' eq $last || 'W' eq $last) {
				$binding{$a} = $col1;
			} else {
				warn "$file:$main::line $a: illegal value for column (last:$last)\n";
				return undef;
			}
			$col1 = $main::token;
			$last = $tok;
			$tok = Token($curfile);
		}
		if ('newline' ne $last) {
			warn "$file: $main::line out of header columns\n";
			do {
				$tok = Token($curfile);
			} while (defined($tok) && 'newline' ne $tok);
		}

		$last = $binding{$key};
		next unless ($add || exists ${$ref}{$last});
		${$ref}{$last} ||= {};
		while (($a, $v) = each %binding) {
			next if (exists ${$ref}{$last}{$a} && ! $overwrite);
			${$ref}{$last}{$a} = $binding{$a};
		}
	}
	close $curfile;
	return $ref;
}

# Read a token from the given filehandle, uses the lastc from main	(ksb)
sub Token($) {
	my($file) = @_;
	my($c, $ret);

	$c = defined($main::lastc) ? $main::lastc : getc($file);
	$main::lastc = undef;
	while (defined($c)) {
		last if ("\n" eq $c || $c =~ m/\S/o);
		$c = getc($file);
	}
	return undef if (!defined($c));
	if ("\n" eq $c) {
		++$main::line;
		$main::token = '';
		return 'newline';
	}
	return $main::token = $c if ($c =~ m/[\%\n.=]/o);
	if ('#' eq $c) {
		$main::token = <$file>;
		chomp $main::token;
		return 'newline';
	}
	if ("'" eq $c) {
		warn "stray \', unbalanced m4 quotes?\n";
		return &Token($file);
	}
	$main::token = '';
	if ('"' eq $c) {
		my $cvt;
		my $errline = $main::line;
		while (defined($c = getc($file))) {
 top:  # octal conversion reads one ahead, so it loops here --ksb
			last if ('"' eq $c);
			if ('\\' ne $c) {
				$main::token .= $c;
				next;
			}
			if (!defined($c = getc($file))) {
				warn "open C string ends with a backslash EOF?\n";
				return undef;
			}
			if ("\n" eq $c) {
				++$main::line;
				next;
			}
			if ("e" eq $c) {
				$main::token .= '\\';
				next;
			}
			if ("n" eq $c) {
				$main::token .= "\n";
				next;
			}
			if ("t" eq $c) {
				$main::token .= "\t";
				next;
			}
			if ("b" eq $c) {
				$main::token .= "\b";
				next;
			}
			if ("r" eq $c) {
				$main::token .= "\r";
				next;
			}
			if ("f" eq $c) {
				$main::token .= "\f";
				next;
			}
			if ("v" eq $c) {
				$main::token .= "\013";
				next;
			}
			# We eat the non-digit character after an octal
			# the octal is not exactly three characters. A goto
			# fixed that.
			if ($c =~ m/[01234567]/o) {
				my $i;
				$i = $c;
				$c = getc($file);
				if (defined($c) && $c =~ m/[01234567]/o) {
					$i .= $c;
					$c = getc($file);
					if (defined($c) && $c =~ m/[01234567]/o) {
						$i .= $c;
						$c = getc($file);
					}
				}
				$main::token .= chr(oct($i));
				if (!defined($c)) {
					warn "open C string ends after backslash-octal with an EOF?\n";
					return undef;
				}
				goto top;
			}
			$main::token .= $c;
		}
		return '"';
	}
	if ('`' eq $c) {
		my $errline = $main::line;
		my $depth = 1;
		while (defined($c = getc($file))) {
			++$main::line if ("\n" eq $c);
			++$depth if ('`' eq $c);
			if ("'" eq $c) {
				--$depth;
				last if (0 == $depth);
			}
			$main::token .= $c;
		}
		if (!defined($c)) {
			warn "end of file in m4 string starting at line $errline\n";
		}
		return '"';
	}
	# otherwise we have a macro or plain word?
	$main::token = $c;
	$ret = $c =~ m/[A-Za-z_]/o ? 'M' : 'W';
	while (defined($c = getc($file))) {
		last if ("=" eq $c || $c =~ m/\s/o);
		$main::token .= $c;
		$ret = 'W' if ($c =~ m/[^A-Za-z0-9_]/o);
	}
	if (defined($c)) {
		$main::lastc = $c;
	}
	return $ret;
}

# the default order to output the hosts for WriteDB			(ksb)
sub HostOrder($ $)
{
	local($a,$b) = @_;
	my($c, $d, $n, $k) = ($a, $b, $a, $b);
	$c =~ s/[0-9.]+.*$//;
	$d =~ s/[0-9.]+.*$//;
	$k =~ s/^[^0-9]+//;
	$n =~ s/^[^0-9]+//;
	$k =~ s/^([0-9]*).*/$1/;
	$n =~ s/^([0-9]*).*/$1/;
	$n = 0 if ('' eq $n);
	$k = 0 if ('' eq $k);
	return $c cmp $d || 0+$n <=> 0+$k;
}

# Output the hash we read, just the marcos listed			(ksb)
sub WriteDB(& $ $ $ @) {
	my $order = shift;
	my $ref = shift;
	my $out = shift;
	my $key = shift;
	my @cols = @_;
	local *CUROUT;

	return undef unless 'HASH' eq ref($ref);
	return undef unless defined($out);
	$key ||= 'HOST';
	$order ||= &HostOrder;

	my($node, $col, $value, $c);
	return undef unless open CUROUT, ">$out";
	print CUROUT '%', join(' ', $key, @cols), "\n";
	my @ordered  = sort { &$order($a, $b); } keys %{$ref};

	my($fM4Ever, $iM4Qs, $iIFS, $iCQs, $iNonPrint);
	foreach $node (@ordered) {
		print CUROUT $node;
		foreach $col (@cols) {
			if ('.' eq $col || !exists(${$ref}{$node}{$col})) {
				print CUROUT ' .';
				next;
			}
			$value = ${$ref}{$node}{$col};
			if ('' eq $value || '.' eq $value) {
				print CUROUT " `$value'";
				next;
			}
			$fM4Ever = $iM4Qs = $iIFS = $iCQs = $iNonPrint = 0;
			foreach $c (split //o, $value) {
				if ($c =~ m/[\s=#]/o) {
					++$iIFS;
				} elsif ($c !~ m/[[:print:]]/o) {
					++$iNonPrint;
				} elsif ($c =~ m/'/o) {
					$fM4Ever = 1, --$iM4Qs;
				} elsif ($c =~ m/`/o) {
					$fM4Ever = 1, ++$iM4Qs;
				} elsif ($c =~ m/["\\]/o) {
					++$iCQs;
				}
			}
			if (!$fM4Ever && 0 == $iCQs && 0 == $iIFS && 0 == $iNonPrint) {
				print CUROUT " $value";
			} elsif (0 == $iM4Qs && 0 == $iNonPrint) {
				print CUROUT " `$value'";
			} elsif (0 == $iCQs && 0 == $iNonPrint) {
				print CUROUT " \"$value\"";
			} else {
				print CUROUT ' "';
				foreach $c (split //o, $value) {
					if ($c eq "\n") {
						$c = '\n';
					} elsif ($c eq "\t") {
						$c = '\t';
					} elsif ($c eq "\b") {
						$c = '\b';
					} elsif ($c eq "\r") {
						$c = '\r';
					} elsif ($c eq "\f") {
						$c = '\f';
					} elsif ($c eq "\013") {
						$c = '\v';
					} elsif ($c =~ m/([\\"'])/o) {
						$c = "\\$1";
					} elsif ($c !~ m/[[:print:]]/o) {
						$c = sprintf "\\%03o", ord($c);
					}
					print CUROUT $c;
				}
				print CUROUT '"';
			}
		}
		print CUROUT "\n";
	}
	close CUROUT;
	return @cols;
}

}
## Example test driver:
##my $cf = "sac.cf";
##my $key = 'HOST';
##my $test = {};
##my $res =  ReadDB($test, $cf, $key, 1, 1);
##die "ReadDB failed for $cf" if (!defined($res));
##$res = WriteDB(sub { HXMD::HostOrder($a, $b); }, $res, "out.cf", $key, 'HOSTTYPE', 'LOC');
##die "can't write out.cf" unless defined($res);
##exit 0;

1;
__END__
# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

HXMD - Perl extension to read/write HXMD configuration files

=head1 VERSION

Version 0.01

=head1 SYNOPSIS

use HXMD;

my $hashref = {};

HXMD::ReadDB($hashref, 'PathToDistrib.cf', 'KeyField', fAdd, fOverwrite);

HXMD::WriteDB(sub { sort-block }, $hashref, 'out.cf', 'KeyField', @cols);

=head1 DESCRIPTION

HXMD provides some basic input and output operations for hxmd (and distrib)
configuration files.  It populates a hash keyed on the value
of the KeyField macro, each value for that key is a hash of
the columns that were specified for the key, as well as any line-based
macros in-scope for any definition of that key.

The datastructure will be :
     $hashref{ValueOfKeyField} =  { COL1 => VAL1,
                                    COL2 => VAL2 }

=head1 FUNCTIONS

=head2 ReadDB

To emulate hxmd's -C option use :
	HXMD::ReadDB($hashref, $opt{'C'}, 'HOST', 1, 1);

To emulate hxmd's -X option use :
	HXMD::ReadDB($hashref, $opt{'X'}, 'HOST', 0, 1);

To emulate hxmd's -Z option use :
	HXMD::ReadDB($hashref, $opt{'Z'}, 'HOST', 1, 0);

After reading all the configuration files specified look up
the HOST "nostromo.npcguild.org"'s "COLOR" value with :
	${$hashref}{'nostromo.npcguild.org'}{'COLOR'}

For debugging you might "use Data::Dumper;" and "warn Dumper $hashref;".

=head2 WriteDB

To output an updated configuration file :
	HXMD::WriteDB(sub { ordered }, $hashref, $outname, 'HOST', @cols);

=head2 HostOrder

Passed to WriteDB this function sorts most hostnames into a reasonable
order.  For example placing "host7" after "host1" and before "host21".

=head2 EXPORT

ReadDB, WriteDB, HostOrder.


=head1 SEE ALSO

If hxmd's the right tool, use it instead.

This is not for the weak of heart, but it does allow a perl
program to read the data out of any hxmd (or distrib)
configuration file.


=head1 AUTHOR

KSB, E<lt>kevin.braunsdorf@gmail.comE<gt>

=head1 SUPPORT

Internal FedEx support from SAC (sac@request.fedex.com).

=head1 ACKNOWLEDGEMENTS

Ed Anderson kicked this off by asking me how to do this in perl.

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2008 by ksb (Kevin S Braunsdorf)

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.8.8 or,
at your option, any later version of Perl you may have available.

=cut
