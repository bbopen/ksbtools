#!/usr/bin/env ksh
# $Id: mreg.ksh,v 1.6 2010/08/12 19:22:53 ksb Exp $
# Regression test for msrc, this should be good.  We'll have to have a
# way to ssh to localhost -- ksb
# This takes between 14 and 20 seconds to run.

unset MSRC HXMD HXMD_PASS
: ${msrc_path:=${1:-msrc}}

$msrc_path -V >/dev/null || exit 1
$msrc_path -h >/dev/null || exit 1
if ! ssh localhost exit 0 ; then
	echo "$0: must be able to ssh to localhost" 1>&2
	exit 5
fi
if ! rdist -V >/dev/null ; then
	echo "$0: must be able to run rdist" 1>&2
	exit 6
fi

MSDIR=/tmp/mrt$((RANDOM%1000)),$$
while ! mkdir -p $MSDIR ; do
	MSDIR=$MSDIR,$((RANDOM%100))
done

cd $MSDIR || exit 102
trap 'if [ -e core ] || [ -e msrc.core ] ; then
		file *core*
		ls -las `pwd`/*core*
		exit 101
	fi
	cd /
	rm -rf $MSDIR' EXIT

set -e
mkdir -p msrc/p1 msrc/p2 src lsrc
cd msrc
cat >p1/msrc.mk <<\!
all:
	echo lower-short

__msrc: all
!
cat >p1/Msrc.mk <<\!
all:
	echo upper-short

__msrc: all
!
cat >p1/makefile <<\!
all:
	echo lower-long

__msrc: all
!
cat >p1/Makefile <<\!
all:
	echo upper-long

alternate:
	echo UPPER-LONG

__msrc: all

__check: alternate

__clean:
	echo clean-works >$@
!
(
echo RSH_PATH=`whence ssh`
echo RDISTD_PATH=`whence rdistd`
echo localhost
)>host.cl

# make hook __msrc
# -f makefile, Msrc.mk msrc.mk makefile Makefile
cd p1
[ _upper-short = _`$msrc_path -f Msrc.mk -C../host.cl :` ] || exit 10
[ _lower-short = _`$msrc_path -f msrc.mk -C../host.cl :` ] || exit 10
[ _lower-long = _`$msrc_path -f makefile -C../host.cl :` ] || exit 10
[ _upper-long = _`$msrc_path -f Makefile -C../host.cl :` ] || exit 10
# deplete the Makefiles to make sure they are scanned for in the documented
# order.
[ _upper-short = _`$msrc_path -C../host.cl :` ] || exit 11
rm Msrc.mk
[ _lower-short = _`$msrc_path -C../host.cl :` ] || exit 11
rm msrc.mk
[ _lower-long = _`$msrc_path -C../host.cl :` ] || exit 11
rm makefile
[ _upper-long = _`$msrc_path -C../host.cl :` ] || exit 11
# check -m while we have a chance
[ _UPPER-LONG = _`$msrc_path -f Makefile -m __check -C../host.cl :` ] || exit 11
[ -f __clean ] && exit 11
[ _UPPER-LONG = _`$msrc_path -f Makefile -m __check:__clean -C../host.cl :` ] || exit 11
[ -f __clean ] || exit 11
rm __clean
rm ../../src/p1/*

# Check -l and mode variable definition
[ _upper-long = _`$msrc_path -l -C../host.cl :` ] || exit 12
# MSRC_MODE=local|remote, and we are on the same host in both cases
cat >Makefile <<\!
all:
	false

__msrc:
!
cat >mode.host <<\!
MSRC_MODE
!
[ _remote = _`$msrc_path -C../host.cl cat mode` ] || exit 13
[ _local = _`$msrc_path -l -C../host.cl cat mode` ] || exit 13
[ _`hostname` = _`$msrc_path -C../host.cl hostname` ] || exit 13
[ _`hostname` = _`$msrc_path -l -C../host.cl hostname` ] || exit 13

# $MSRC honored
[ _$MSDIR/src/p1 = _`MSRC="" $msrc_path -C../host.cl pwd` ] || exit 15
[ _$MSDIR/src/p1 != _`MSRC="-l" $msrc_path -C../host.cl pwd` ] || exit 15

# Try a different argv[0], we can't need the command "bletch" to work.
# the make hook __msrc is toxic now, and we need mode set in BLETCH_MODE
ln -s `whence $msrc_path` $MSDIR/bletch
cat >mode.host <<\!
BLETCH_MODE
!
cat >Makefile <<\!
all:
	false

__msrc: all
!
# This file would usually be in hxmd's lib, but we can't write it there.
# It'll be found in the current directory, just the same (we hope).
echo "RDIST_PATH=\`rdist >/dev/null'" >bletch.cf
[ _remote = _`$MSDIR/bletch -C../host.cl cat mode` ] || exit 21
sync
[ _local = _`$MSDIR/bletch -l -C../host.cl cat mode` ] || exit 21

# Remove makefile poison and links to msrc we don't need.
rm $MSDIR/bletch bletch.cf mode.host

# can mode be read from the makefile?
cat >Makefile <<\!
MODE=local
!
[ _$MSDIR/src/p1 != _`$msrc_path -C../host.cl pwd` ] || exit 22

cat >Makefile <<\!
# an empty file works for me
!

# Find a *.hxmd file and put it in HXMD_PASS?
cat >Msrc.hxmd <<\!
-DCOLOR=red
!
echo "COLOR" > paint.host
[ _red = _`$msrc_path -C../host.cl cat paint` ] || exit 23
# does -z still pass that file, we hope not?
[ _COLOR = _`$msrc_path -z -C../host.cl cat paint` ] || exit 24
[ _blue = _`$msrc_path -z -DCOLOR=blue -C../host.cl cat paint` ] || exit 24
# Since $HXMD_PASS is processed before explicit -D's we can override
# the file without the -z as well. --ksb
[ _green = _`$msrc_path -DCOLOR=green -C../host.cl cat paint` ] || exit 24

# check -y
rm Msrc.hxmd paint.host
cat >Makefile <<\!
SEND=output
PHASE=one
output:
	echo ${PHASE} >$@

__msrc: output
!
rm -f ../../src/p1/* output
[ _one = _`$msrc_path -C../host.cl cat output` ] || exit 25
rm -f ../../src/p1/* output
[ _one = _`$msrc_path -y OTHER=two -C../host.cl cat output` ] || exit 25
rm -f ../../src/p1/* output
[ _two = _`$msrc_path -y PHASE=two -C../host.cl cat output` ] || exit 25
rm -f ../../src/p1/* output

# try -y with a stdout version (do not build a file)
cat >Makefile <<\!
IGNORE=output
PHASE=one
output:
	echo ${PHASE}

__msrc: output
!
[ _one = _`$msrc_path -C../host.cl :` ] || exit 26
[ _one = _`$msrc_path -y OTHER=two -C../host.cl :` ] || exit 26
[ _two = _`$msrc_path -y PHASE=two -C../host.cl :` ] || exit 26

# check ${1} to ${5} in the two different cases (local and remote modes)
# make the INIT_CMD output them in files and abort the update, then
# compare the output values to known good/bad ones.
rm Makefile
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
[ _Fini = _`$msrc_path -C../host.cl -D INIT_CMD="include(my.m4)" :` ] || exit 27
[ _. = _`cat 1` -a \
	_Makefile = _`cat 2` -a \
	_remote = _`cat 3` -a \
	_localhost = _`cat 4` -a \
	_$MSDIR/src/p1 = _`cat 5` ]  || exit 27
rm [12345]
mv Makefile msrc.mk
[ _Fini = _`$msrc_path -l -C../host.cl -D INIT_CMD="include(my.m4)" :` ] || exit 27
[ _. != _`cat 1` -a \
	_msrc.mk = _`cat 2` -a \
	_local = _`cat 3` -a \
	_localhost = _`cat 4` -a \
	_$MSDIR/src/p1 = _`cat 5` ]  || exit 27
rm [12345]

# Not checked:							(LLL or never)
#  the optimization for -P1 vs -Pmany (include ptbw process or not)
#  -F not allowed, because when passed in HXMD (or HXMD_PASS) we'd
#	not catch it anyway

# did we ever drop core?
[ -f core -o -f msrc.core ] && exit 99

exit 0
