#!/usr/bin/env ksh
# $Id: hreg.ksh,v 1.18 2010/07/22 20:58:10 ksb Exp $
# Regression test for hxmd -- this is not going to be easy		(ksb)
# This should have little time to run (maybe 3 seconds) and will exit
# 0 on success (with no output) and non-zero to inidicate which
# test block failed.  Like all of these this is not exhaustive,  but
# provides some protection from bugs I've seen before.

: ${hxmd_path:=${1:-hxmd}}

$hxmd_path -V >/dev/null ||exit 1
$hxmd_path -h >/dev/null ||exit 1

# we don't test all the xapply pass-throughs
HXDIR=/tmp/hrt$((RANDOM%1000)),$$
while ! mkdir -p $HXDIR ; do
	HXDIR=$HXDIR,$((RANDOM%100))
done
trap 'if [ -e core ] || [ -e hxmd.core ] ; then
		file *core*
		ls -las `pwd`/*core*
		exit 80
	else
		cd /
		rm -rf $HXDIR
	fi' EXIT

cd $HXDIR
NOW=`date +%Y%m%d`
echo $NOW >now
cat <<\! >one.cl
localhost
!
# There is a space on the end of the nostromo line below, and a tab on
# the end of the space line, all by intent. -- ksb
cat <<\! >two.cf
%HOST COLOR MAX			RAISE

localhost       .	10  	.	
nostromo	grey	.	. 
space		.	. 	localhost	
!
cat <<\! >zero.cf
COLOR=`red'
!
mkdir incs
cat <<\! >incs/gFunk.m4
define(MIN,3)dnl
!
cat <<\! >cmd.0
#!/usr/bin/env sh
echo "$@"
!

# -C file.cf
[ _"localhost" = _`$hxmd_path -Cone.cl 'echo HOST'` ] || exit 2
[ _"localhostnostromospace" = _`$hxmd_path -Ctwo.cf 'echo HOST' |tr -d '\n\r'` ] || exit 2
[ _$PWD/one.cl = _`$hxmd_path -Cone.cl 'echo HXMD_OPT_C'` ] || exit 2
[ _HXMD_OPT_X = _`$hxmd_path -Cone.cl 'echo HXMD_OPT_X'` ] || exit 2
[ _HXMD_OPT_Z = _`$hxmd_path -Cone.cl 'echo HXMD_OPT_Z'` ] || exit 2
[ _$PWD/two.cf = _`$hxmd_path -Ctwo.cf 'echo HXMD_OPT_C' | uniq` ] || exit 2
[ _$PWD/one.cl:$PWD/two.cf = _`$hxmd_path -Cone.cl:two.cf 'echo HXMD_OPT_C' | uniq` ] || exit 2

# -Z file.cf
[ _"COLOR" = _`$hxmd_path -Cone.cl 'echo COLOR'` ] || exit 3
[ _"red" = _`$hxmd_path -Zzero.cf -Cone.cl 'echo COLOR'` ] || exit 3
[ _"COLORgreyCOLOR" = _`$hxmd_path -Ctwo.cf 'echo COLOR' | tr -d '\r\n'` ] || exit 3
[ _"redgreyred" = _`$hxmd_path -P1 -Zzero.cf -Ctwo.cf 'echo COLOR' |tr -d '\r\n'` ] || exit 3

# -X file.cf
[ _"MAX" = _`$hxmd_path -Cone.cl 'echo MAX'` ] || exit 4
[ _"10" = _`$hxmd_path -Cone.cl -Xtwo.cf 'echo MAX'` ] || exit 4
[ _"HXMD_OPT_X" = _`$hxmd_path -Cone.cl 'echo HXMD_OPT_X'` ] || exit 4
[ _$PWD/two.cf = _`$hxmd_path -Xtwo.cf -Cone.cl 'echo HXMD_OPT_X'` ] || exit 4

# -Y top, trivial case first, we'll come back to this one
[ _"" = _`$hxmd_path -Y'divert(-1)dnl' -Cone.cl 'echo MAX'` ] || exit 5

# same trivial test for -j m4prep
echo "divert(-1)dnl" >break.m4
[ _"" = _`$hxmd_path -j break.m4 -Cone.cl 'echo MAX'` ] || exit 5
rm break.m4

# m4 options -D, -U, -I
[ _"99" = _`$hxmd_path -Cone.cl -DMAX=99 'echo MAX'` ] || exit 5
[ _"3/MAX" = _`$hxmd_path -Cone.cl -I$PWD/incs 'include(gFunk.m4)echo MIN/MAX'` ] || exit 5
[ _"ONE/MAX" = _`$hxmd_path -Cone.cl -DONE=1 -UONE -I$PWD/incs 'include(gFunk.m4)echo ONE/MAX'` ] || exit 5
[ _/tmp/missing = _`HXMD_PATH=./missing:/tmp/missing:/usr/bin $hxmd_path -V | sed -n -e 's/.*double-dash.*: //p'` ] || exit 5
[ _$HXDIR = _`HXMD_PATH=./missing:$HXDIR:/tmp/missing:/usr/bin $hxmd_path -V | sed -n -e 's/.*double-dash.*: //p'` ] || exit 5
[ _$NOW = _`$hxmd_path -Cone.cl -I $HXDIR "echo include(now)"` ] || exit 5
[ _$NOW = _`HXMD_PATH=$HXDIR $hxmd_path -C$PWD/one.cl -I -- "echo include(now)"` ] || exit 5
[ _Cache.m4 = _`$hxmd_path -V |sed -n -e 's/.*cache recipe file: *//p'` ] || exit 6

# -F literal checks
chmod -x cmd.0
$hxmd_path -Cone.cl -F0 cmd.0 two.cf >r1 2>r2
[ 0 -ne $? ] && [ ! -s r1 ] && grep "must be execut" r2 >/dev/null || exit 10
chmod +x cmd.0
$hxmd_path -Cone.cl -F0 cmd.0 two.cf >r1 2>r2
[ 0 -eq $? ] && [ ! -s r2 ] && grep "/two.cf\$" r1 >/dev/null || exit 10
# try the otherway (more literals)
[ _local = _`$hxmd_path -Cone.cl -F2 'echo %(1,1-5)' HOST` ] || exit 11

# files run through m4
[ _1 = _`$hxmd_path -Cone.cl 'echo HXMD_C'` ] || exit 11
[ _2 = _`$hxmd_path -Cone.cl 'echo HXMD_C%0' incs/gFunk.m4 ` ] || exit 11

# -k HOST
sed -e 's/%HOST/%NODE/' < two.cf >r1
[ _"HOST" = _`$hxmd_path -k NODE -Cone.cl 'echo HOST'` ] || exit 2
[ _"HOSTHOSTHOST" = _`$hxmd_path -k NODE -Cr1 'echo HOST' |tr -d '\n\r'` ] || exit 12
[ _"localhost" = _`$hxmd_path -k NODE -Cone.cl 'echo NODE'` ] || exit 2
[ _"localhostnostromospace" = _`$hxmd_path -P1 -k NODE -Cr1 'echo NODE' |tr -d '\n\r'` ] || exit 12


# -B macro -B int, and counters
# HXMD_U_COUNT and U_SELECTED via -B
[ 1 -eq `$hxmd_path -Cone.cl 'echo HXMD_U_COUNT'` ] || exit 13
[ 3 -eq `$hxmd_path -Ctwo.cf 'echo HXMD_U_COUNT' | uniq` ] || exit 13
[ 1 -eq `$hxmd_path -Cone.cl 'echo HXMD_U_SELECTED'` ] || exit 13
[ 3 -eq `$hxmd_path -Ctwo.cf 'echo HXMD_U_SELECTED' | uniq` ] || exit 13
[ 1 -eq `$hxmd_path -Cone.cl -B1 'echo HXMD_U_SELECTED'` ] || exit 13
[ 3 -eq `$hxmd_path -Ctwo.cf -BMAX 'echo HXMD_U_COUNT'` ] || exit 13
[ 1 -eq `$hxmd_path -Ctwo.cf -BMAX 'echo HXMD_U_SELECTED'` ] || exit 13
[ 1 -eq `$hxmd_path -Ctwo.cf -BMAX 'echo HXMD_B'` ] || exit 13
[ 2 -eq `$hxmd_path -Ctwo.cf:one.cl -BMAX 'echo HXMD_B'` ] || exit 13
[ 2 -eq `$hxmd_path -Ctwo.cf -Cone.cl -BMAX 'echo HXMD_B'` ] || exit 13
[ 2 -eq `$hxmd_path -Ctwo.cf -Cone.cl -B !MAX 'echo HOST' |wc -l` ] || exit 13
[ 0 -eq `$hxmd_path -Ctwo.cf:one.cl -Zzero.cf -B !COLOR 'echo HOST' |wc -l` ] || exit 13
[ 0 -eq `$hxmd_path -Ctwo.cf -Cone.cl -BMAX -BCOLOR 'echo HXMD_B' |wc -l` ] || exit 13

# -E string or math expr
# HXMD_U_COUNT via -E
[ 3 -eq `$hxmd_path -Ctwo.cf 'echo HXMD_U_COUNT' |uniq` ] || exit 14
[ 1 -eq `$hxmd_path -Ctwo.cf 'echo HXMD_B' |uniq` ] || exit 14
[ 1 -eq `$hxmd_path -Ctwo.cf -Cone.cl -EHOST=space 'echo HOST' |wc -l` ] || exit 13
[ 2 -eq `$hxmd_path -Ctwo.cf -Cone.cl -EHOST!space 'echo HOST' |wc -l` ] || exit 13
[ 1 -eq `$hxmd_path -Ctwo.cf -Cone.cl -Espace=HOST 'echo HOST' |wc -l` ] || exit 13
[ 2 -eq `$hxmd_path -Ctwo.cf -Cone.cl -Espace!HOST 'echo HOST' |wc -l` ] || exit 13
[ 1 -eq `$hxmd_path -Ctwo.cf -E10=MAX 'echo HXMD_U_SELECTED' |uniq` ] || exit 14
[ 3 -eq `$hxmd_path -Ctwo.cf -E100!MAX 'echo HXMD_U_SELECTED' |uniq` ] || exit 14
[ 3 -eq `$hxmd_path -Ctwo.cf -E!100=MAX 'echo HXMD_U_SELECTED' |uniq` ] || exit 14
[ 1 -eq `$hxmd_path -Ctwo.cf -E!10!MAX 'echo HXMD_U_SELECTED' |uniq` ] || exit 14
[ 1 -eq `$hxmd_path -Ctwo.cf -E10==MAX 'echo HXMD_U_SELECTED' |uniq` ] || exit 14
[ 1 -eq `$hxmd_path -Ctwo.cf -E11!=MAX 'echo HOST' |wc -l` ] || exit 14
[ 1 -eq `$hxmd_path -Ctwo.cf -E"11==incr(MAX)" 'echo HOST' |wc -l` ] || exit 14
[ 1 -eq `$hxmd_path -Ctwo.cf -E"5<MAX" 'echo HOST' |wc -l` ] || exit 14
[ 1 -eq `$hxmd_path -Ctwo.cf -E"10<=MAX" 'echo HOST' |wc -l` ] || exit 14
[ 0 -eq `$hxmd_path -Ctwo.cf -E"4>MAX" 'echo HOST' |wc -l` ] || exit 14
[ 0 -eq `$hxmd_path -Ctwo.cf -E"9>=MAX" 'echo HOST' |wc -l` ] || exit 14
$hxmd_path -Ctwo.cf '[ %u -eq HXMD_U ] || echo HOST' >r1 2>r2
[ 0 -eq $? -a ! -s r1 -a ! -s r2 ] || exit 14

# -G guard
[ _localhost = _`$hxmd_path -Ctwo.cf -G RAISE 'echo HOST'` ] || exit 15
[ 11 -eq `$hxmd_path -Ctwo.cf -G RAISE 'echo incr(MAX)'` ] || exit 15
[ _grey = _`$hxmd_path -Ctwo.cf -G "ifelse(MAX,10,\\\`nostromo')" 'echo COLOR'` ] || exit 15
# use the guard to sort the hosts, those with a color first
[ _nostromolocalhostspace = _`$hxmd_path -Ctwo.cf -G "divert(ifdef(\\\`COLOR',3,7))HOST" 'echo HOST' | tr -d ' \n\r'` ] || exit 15

# -K rerun filter
# -r rerun command
$hxmd_path -K 'cat HXMD_0' -r 'HOST exits HXMD_STATUS (%u HXMD_U)' -Q "#comment" \
	-Cone.cl 'exit 7' >r1 2>r2
[ 0 -eq $? -a  ! -s r1 ] && fgrep "localhost exits 7 (%u 0)" r2 >/dev/null || exit 20
# -Q included
fgrep "#comment" r2 >/dev/null || exit 21
$hxmd_path -K '|cat' -r 'HOST exits HXMD_STATUS (%u HXMD_U)' -Q "#comment" \
	-Cone.cl -F2 'exit 7' 'm4exit(10)' >r1 2>r2
[ 0 -eq $? -a ! -s r1 ] && grep "exit.* 10 .*skip.*" r2 >/dev/null || exit 22
fgrep "#comment" r2 >/dev/null || exit 23
fgrep " 1010 " r2 >/dev/null || exit 24

# handle 4 stdin uses (-C, -X, -Z, file), same checks we used but - for stdin
[ 2 -eq `$hxmd_path -Ctwo.cf:- -BMAX 'echo HXMD_B' <one.cl` ] || exit 30
[ 2 -eq `$hxmd_path -C-:one.cl -BMAX 'echo HXMD_B' <two.cf` ] || exit 30
[ 2 -eq `$hxmd_path -Ctwo.cf -C- -BMAX 'echo HXMD_B' <one.cl` ] || exit 30
[ 2 -eq `$hxmd_path -C- -Cone.cl -BMAX 'echo HXMD_B' <two.cf` ] || exit 30
[ 0 -eq `$hxmd_path -Ctwo.cf:one.cl -Z - -B !COLOR 'echo HOST' <zero.cf |wc -l` ] || exit 30
[ _"10" = _`$hxmd_path -Cone.cl -X- 'echo MAX' <two.cf` ] || exit 30
$hxmd_path -Cone.cl -F0 - two.cf <cmd.0 >r1 2>r2
[ 0 -eq $? ] && [ ! -s r2 ] && grep "/two.cf\$" r1 >/dev/null || exit 30

# -o attributes
cat >r1 <<\!
COLOR=cream
%HOST COLOR MAX RAISE
localhost . 10 .
nostromo grey . .
space . . localhost
!
$hxmd_path -DCOLOR=cream -o 'COLOR MAX RAISE' -Cone.cl:two.cf -K 'cmp HXMD_U_MERGED r1' ':' || exit 40

# -z and HXMD/HXMD_PASS checks
# This is tricky to shell/m4 quote: we use to-open and to-close $TO $TC below
# to make this a little easier.  We need to see if $HXMD_PASS is removed
# from a nested hxmd's envronment, and it HXMD takes its place in that case.
# (In most cases the nested hxmd would be in a script call indirectly. -- ksb)
TO="\`" TC="'"
[ _-DFOO=OK = _`HXMD_PASS=-DFOO=OK $hxmd_path -Cone.cl 'echo $HXMD_PASS'` ] || exit 41
[ _ = _`HXMD_PASS=-DFOO=OK $hxmd_path -z -Cone.cl 'echo $HXMD_PASS'` ] || exit 41
[ _OK = _`HXMD_PASS=-DFOO=OK $hxmd_path -Cone.cl 'echo FOO'` ] || exit 41
[ _OK = _`HXMD_PASS=-DFOO=OK $hxmd_path -z -Cone.cl 'echo FOO'` ] || exit 41
[ _-DFOO=OK = _`HXMD_PASS=-DFOO=OK $hxmd_path -Ctwo.cf -E HOST=nostromo "$hxmd_path -Cone.cl 'echo \\\$HXMD_PASS'"` ] || exit 43
[ _ = _`HXMD_PASS=-DFOO=OK $hxmd_path -Ctwo.cf -E HOST=nostromo "$hxmd_path -z -Cone.cl 'echo \\\$HXMD_PASS'"` ] || exit 43
[ _ = _`HXMD_PASS=-DFOO=OK $hxmd_path -z -Ctwo.cf -E HOST=nostromo "$hxmd_path -Cone.cl 'echo \\\$HXMD_PASS'"` ] || exit 43
[ _ = _`HXMD_PASS=-DFOO=OK $hxmd_path -z -Ctwo.cf -E HOST=nostromo "$hxmd_path -z -Cone.cl 'echo \\\$HXMD_PASS'"` ] || exit 43
[ _OK = _$(HXMD_PASS=-DFOO=OK $hxmd_path -Ctwo.cf -E HOST=nostromo "${TO}$hxmd_path -Cone.cl \"echo FOO\"${TC}") ] || exit 44
[ _FOO = _$(HXMD_PASS=-DFOO=OK $hxmd_path -z -Ctwo.cf -E HOST=nostromo "${TO}$hxmd_path -Cone.cl \"echo FOO\"${TC}") ] || exit 44
[ _FOO = _$(HXMD_PASS=-DFOO=OK $hxmd_path -z -Ctwo.cf -E HOST=nostromo "${TO}$hxmd_path -Cone.cl \"echo FOO\"${TC}") ] || exit 44
[ _FOO = _$(HXMD_PASS=-DFOO=OK $hxmd_path -z -Ctwo.cf -E HOST=nostromo "${TO}$hxmd_path -Cone.cl \"echo FOO\"${TC}") ] || exit 44
# ... and the fall-back to HXMD checks:
[ _OK = _$(HXMD=-DFOO=DEF HXMD_PASS=-DFOO=OK $hxmd_path -Ctwo.cf -E HOST=nostromo "${TO}$hxmd_path -Cone.cl \"echo FOO\"${TC}") ] || exit 45
[ _DEF = _$(HXMD=-DFOO=DEF HXMD_PASS=-DFOO=OK $hxmd_path -z -Ctwo.cf -E HOST=nostromo "${TO}$hxmd_path -Cone.cl \"echo FOO\"${TC}") ] || exit 45
[ _DEF = _$(HXMD=-DFOO=DEF HXMD_PASS=-DFOO=OK $hxmd_path -z -Ctwo.cf -E HOST=nostromo "${TO}$hxmd_path -Cone.cl \"echo FOO\"${TC}") ] || exit 45
[ _DEF = _$(HXMD=-DFOO=DEF HXMD_PASS=-DFOO=OK $hxmd_path -z -Ctwo.cf -E HOST=nostromo "${TO}$hxmd_path -Cone.cl \"echo FOO\"${TC}") ] || exit 45
unset HXMD_PASS HXMD

# The new cache feature, when a directory is presented as a mapped file we
# m4 filter "Cache.m4" from that directory to build a recipe file to update
# the cache for the given host context, then run:
#	cd $dir && make -[s]f $Cache $(basename $f)
# where basename removes the leading path, and any leading dots, and removes
# the last dor suffix.  Which sounds odd, but works out.  So "dns.host" makes
# "dns" , as does "/home/ab/.dns.host".  If the rules produce the empty string
# we update the name of the key macro (viz. /tmp/cache1/.).
# The make target must output the requested data on stdout, not update some
# file and leave it in the cache (rather update then marshal it to stdout).
rm -f now r1 r2 cmd.0
mkdir cache
ln -s cache .owner
cat >cache/Cache.m4 <<\!
dnl msrc hook to acces the Death Bird cache directory
`# $Id: hreg.ksh,v 1.18 2010/07/22 20:58:10 ksb Exp $

# Called by dirname ($PWD/cache), this allow the cache a "primary operation"
# and is usually the first target in this file
cache:
	echo "the 'PET`"

# Called by symbolic link ($PWD/.owner), this allows a cache to
# support more than one generic operation.
owner:
	echo "Nathan Stack"

# Called by dir/ or dir/., this allows the cache a per-host recipe, with
# some care. For this test I just hard-coded two hosts, which is generally
# bad style -- use markup on the managed attributes of the host.
nostromo:
	echo "This is for Mark Twain"

mother:
	echo "Use that hypo and let me out of here."

!
# Work around lame gnu m4 (missing the "paste" built-in)
if [ _"" != _`echo "defn(\\\`paste')" | m4` ] ; then
	echo "'paste(HXMD_CACHE_DIR\`/Control')dnl"
else
	echo "'include(HXMD_CACHE_DIR\`/Control')dnl"
fi >>cache/Cache.m4
cat >cache/Control <<\!
# regression cache control recipe: setup (init), purge, and so on.
init: state

purge: FRC
	rm -f state FRC

count: state FRC
	read C Junk <state && expr $${C:-1} + 1 >state && echo $$C

state:
	echo "1" >$@

FRC:
!
[ _"the dog" = _"`$hxmd_path -DPET=dog -Ctwo.cf -E HOST=nostromo cat cache`" ] || exit 56
[ _"Nathan Stack" = _"`$hxmd_path -DPET=not -Ctwo.cf -E HOST=nostromo cat .owner`" ] || exit 57
[ _"This is for Mark Twain" = _"`$hxmd_path -DPET=not -Ctwo.cf -E HOST=nostromo cat cache/.`" ] || exit 57

# Test the derived state feature, build a counter and count to three
ln -s cache .count
(cd cache && make -s -f Control init)
[ 1 -eq `$hxmd_path -Ctwo.cf -E HOST=nostromo cat .count` ] || exit 58
[ 2 -eq `$hxmd_path -Ctwo.cf -E HOST=nostromo cat .count` ] || exit 58
[ 3 -eq `$hxmd_path -Ctwo.cf -E HOST=nostromo cat .count` ] || exit 58
# example cache clean, just FYI, purge is better than clean, I think.
(cd cache && make -s -f Control purge)
rm .count .owner
rm -rf cache


# Previous bugs
# The C string code used to eat the character after a "short" octal conversion
cat >octal.cf <<\!
SERVICES="\60\041"
octal
!
[ _"octal 0!" = _"$($hxmd_path -Coctal.cf 'echo HOST SERVICES')" ] || exit 63

# I made HXMD_C wrong in every file but the first once (never release)
[ _"3 3" = _"`$hxmd_path -Coctal.cf -F 3 echo HXMD_C HXMD_C`" ] || exit 62
rm octal.cf

# -Z trailing host bug check, which is a real botch in hostdb < 1.69 --ksb
[ _"OK" = _"`$hxmd_path -Z - -C one.cl -E HOST=localhost 'echo OK' <<\!
%HOST OK
anyhost ouch
!
`" ] || exit 61

# Bug: -N command run when config file is noexistent file.
if [ ! -z "`$hxmd_path -N '%0date' -C this/file/not/found.cf 2>/dev/null 'echo Bad too' ; sleep 1`" ] ; then
	exit 60
fi

# Test for -s (to turn off slow start)
# This would require datecalc or something like that (at least perl -e) skip
# this one for now.  And I'm pretty sure it works, since it breaks ssh. :-)

exit 0
