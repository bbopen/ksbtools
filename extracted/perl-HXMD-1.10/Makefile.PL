# $Id: Makefile.PL,v 1.2 2008/09/29 15:27:14 ksb Exp $
use strict;
use warnings;
use ExtUtils::MakeMaker;

WriteMakefile(
    NAME                => 'HXMD',
    AUTHOR              => 'KS Braunsdorf <kevin.braunsdorf@gmail.com>',
    VERSION_FROM        => 'lib/HXMD.pm',
    ABSTRACT_FROM       => 'lib/HXMD.pm',
    PL_FILES            => {},
    PREREQ_PM => {
        'Test::More' => 0,
    },
    dist                => { COMPRESS => 'gzip -9f', SUFFIX => 'gz', },
    clean               => { FILES => 'HXMD-*' },
);
