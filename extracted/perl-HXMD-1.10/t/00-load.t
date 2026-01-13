#!perl -T

use Test::More tests => 1;

BEGIN {
	use_ok( 'HXMD' );
}

diag( "Testing HXMD $HXMD::VERSION, Perl $], $^X" );
