#!/usr/bin/env ksh
# $Id: mmreg.ksh,v 1.4 2010/08/14 18:50:17 ksb Exp $
# Regression test for mmsrc, this should look like msrc's.

unset MMSRC MMSRC_PASS
: ${mmsrc_path:=${1:-mmsrc}}

$mmsrc_path -V >/dev/null || exit 1
$mmsrc_path -h >/dev/null || exit 1

MMDIR=/tmp/mmrt$((RANDOM%1000)),$$
while ! mkdir -p $MMDIR ; do
	MMDIR=$MMDIR,$((RANDOM%100))
done

cd $MMDIR || exit 102
trap 'if [ -e core ] || [ -e mmsrc.core ] ; then
		file *core*
		ls -las `pwd`/*core*
		exit 101
	fi
	cd /
	rm -rf $MMDIR' EXIT

# check that we get the msrc prereq emulation the same as msrc
[ x__msrc = x`$mmsrc_path -V | sed -n -e 's/.*make hook: *//p'` ] || exit 6
[ x__mmsrc = x`$mmsrc_path -m :: -V | sed -n -e 's/.*make hook: *//p'` ] || exit 6
[ x__msrc:check = x`$mmsrc_path -m :check -V | sed -n -e 's/.*make hook: *//p'` ] || exit 6
[ x__mmsrc:check = x`$mmsrc_path -m :: -m :check -V | sed -n -e 's/.*make hook: *//p'` ] || exit 6

mkdir -p msrc/p1 msrc/p2 src
cd msrc
cat >p1/msrc.mk <<\!
lower-short:
	@echo $@
	@touch $@

__msrc: lower-short
!
cat >p1/Msrc.mk <<\!
upper-short:
	@echo $@
	@touch $@

__msrc: upper-short
!
cat >p1/makefile <<\!
lower-long:
	@echo $@
	@touch $@

__msrc: lower-long
!
cat >p1/Makefile <<\!
upper-long:
	@echo $@
	@touch $@

__msrc: upper-long
!
cat >host.cl <<\!
localhost
!

# make hook __msrc
# -f makefile, Msrc.mk msrc.mk makefile Makefile
cd p1
[ _lower-short = _`$mmsrc_path -f msrc.mk -C../host.cl :` ] || exit 10
[ _upper-short = _`$mmsrc_path -f Msrc.mk -C../host.cl :` ] || exit 10
[ _lower-long = _`$mmsrc_path -f makefile -C../host.cl :` ] || exit 10
[ _upper-long = _`$mmsrc_path -f Makefile -C../host.cl :` ] || exit 10
rm upper-* lower-* ../../src/p1/*

# deplete the Makefiles to make sure they are scanned for in the documented
# order.
[ _upper-short = _`$mmsrc_path -C../host.cl :` ] || exit 11
rm Msrc.mk upper-short ../../src/p1/*
[ _lower-short = _`$mmsrc_path -C../host.cl :` ] || exit 11
rm msrc.mk lower-short ../../src/p1/*
[ _lower-long = _`$mmsrc_path -C../host.cl :` ] || exit 11
rm makefile lower-long ../../src/p1/*
[ _upper-long = _`$mmsrc_path -C../host.cl :` ] || exit 11
rm Makefile upper-long ../../src/p1/*

# $MMSRC honored, these files send eachother, but not themselves
cat >makefile <<\!
CHECK=one two \
	`echo three`
SEND=Makefile
IGNORE=makefile

all:
	echo makefile

__msrc:
!
cat >Makefile <<\!
CHECK=one \
	two three
SEND=makefile
IGNORE=Makefile

all:
	echo Makefile

__msrc:
!
[ _makefile = _`MMSRC="-f Makefile" $mmsrc_path -C../host.cl make -s all` ] || exit 12
rm ../../src/p1/*
[ _Makefile = _`MMSRC="-f makefile" $mmsrc_path -C../host.cl make -s all` ] || exit 12

# check -b
[ _one_two_three_ = _"`$mmsrc_path -f Makefile -b CHECK | tr '\\n' _`" ] || exit 21
[ _one_two_three_ = _"`$mmsrc_path -f makefile -b CHECK | tr '\\n' _`" ] || exit 21
rm ../../src/p1/* Makefile makefile

# Find a *.hxmd file and put it in MMSRC_PASS to show how that works
cat >Msrc.mk <<\!
# an empty recipe files should work for the rest --ksb
!
cat >Msrc.hxmd <<\!
-DCOLOR=red
!
cat >paint.host <<\!
COLOR
!
[ _red = _`$mmsrc_path -C../host.cl cat paint` ] || exit 23
# does -z still pass that file, we hope not?
[ _COLOR = _`$mmsrc_path -z -C../host.cl cat paint` ] || exit 24
[ _blue = _`$mmsrc_path -z -DCOLOR=blue -C../host.cl cat paint` ] || exit 24
# Since $HXMD_PASS is processed before explicit -D's we can override
# the file without the -z as well. --ksb
[ _green = _`$mmsrc_path -DCOLOR=green -C../host.cl cat paint` ] || exit 24
rm paint.host Msrc.hxmd Msrc.mk

# compare the output values to known good/bad ones.
cat >Makefile <<\!
# empty is best for this test
!
cat >my.m4 <<\!
`echo ${1} >1
echo ${2} >2
echo ${3} >3
echo ${4} >4
echo ${5} >5
echo Fini
exit 0
'dnl
!
[ _Fini = _`$mmsrc_path -C../host.cl -D INIT_CMD="include(my.m4)" :` ] || exit 27
[ _$MMDIR/src/p1 == _`cat 1` -a \
	_Makefile = _`cat 2` -a \
	_local = _`cat 3` -a \
	_localhost = _`cat 4` -a \
	_$MMDIR/src/p1 = _`cat 5` ]  || exit 27
rm [12345] my.m4 Makefile

# Not checked, and I don't think I need to:
#  the optimization for -P1 vs -Pmany (include ptbw process or not)
#  -F not allowed, because when passed in HXMD (or HXMD_PASS) we'd
#	not catch it anyway

exit 0
