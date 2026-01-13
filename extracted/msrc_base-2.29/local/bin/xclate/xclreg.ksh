#!/usr/bin/env ksh
# $Id: xclreg.ksh,v 2.12 2008/10/02 14:20:48 ksb Exp $
# Regression test xclate V2.39 (ksb)
# The output should look sane and exit 0, same output on all OS types.
# It takes about 29 to 60 seconds to run, run with -x to watch.		(ksb)
SHELL=`whence sh`
: ${xclate_path:=`whence xclate`}
export SHELL xclate_path

unset xcl_0 xcl_1 xcl_2 xcl_d xcl_link xcl_list
unset ptbw_0 ptbw_1 ptbw_2 ptbw_d ptbw_link ptbw_list

: ${TMPDIR:=/tmp}
while T1=$TMPDIR/xat$$,e$((RANDOM%1000)) ; do
	[ -f $T1 ] || break
done
while T2=$TMPDIR/xat$$,o$((RANDOM%1000)) ; do
	[ -f $T2 ] || break
done
export T1 T2
trap 'rm -f $T1 $T2' EXIT

set -e
# overhead checks
$xclate_path -V >/dev/null
$xclate_path -h >/dev/null

# check that we can fetch version from ourself, and see the env links
env PATH="$PATH" $xclate_path -m -- \
	$xclate_path -vV 2>&1 |grep '\[target\]' >/dev/null

# try for nothing
$xclate_path -m true
$xclate_path -m sh -c "$xclate_path xid sleep 1" </dev/null
$xclate_path -m sh -c "sleep 1 | $xclate_path xid" </dev/null

# try -s trivial case, no output from a single client
[ 0 -eq `$xclate_path -m sh -c "$xclate_path -s -T %x -H end silent_p true" |wc -l` ]
[ 3 -eq `$xclate_path -m sh -c "$xclate_path -s -T %x -H end words_p date" |wc -l` ]
[ 0 -eq `$xclate_path -m sh -c "true |$xclate_path -s -T %x -H end silent_f" |wc -l` ]
[ 3 -eq `$xclate_path -m sh -c "date |$xclate_path -s -T %x -H end words_f" |wc -l` ]
# make the client read -s from the master (xclate 2.49 or better, proto 0.7)
[ 0 -eq `$xclate_path -m -s sh -c "$xclate_path -T %x -H end silent_p true" |wc -l` ]
[ 3 -eq `$xclate_path -m -s sh -c "$xclate_path -T %x -H end words_p date" |wc -l` ]
[ 0 -eq `$xclate_path -m -s sh -c "true |$xclate_path -T %x -H end silent_f" |wc -l` ]
[ 3 -eq `$xclate_path -m -s sh -c "date |$xclate_path -T %x -H end words_f" |wc -l` ]

# try sending data through cmd-mode
echo data | $xclate_path -m -- \
	$xclate_path cmd-mode sh -c 'echo client; read X; echo $X; echo end' >$T1
set _ `cat <$T1`
shift
[ "client data end" = "$*" ]

# try sending data through filter-mode
echo data | $xclate_path -m -- \
	sh -c '(echo client; read X; echo $X; echo end) |$xclate_path cmd-mode' >$T1
set _ `cat <$T1`
shift
[ "client data end" = "$*" ]

# test the whole locking thing
# first test, multilines in parallel
$xclate_path -m -- sh -c "
	for i in 1 2 1 2 0; do echo \$i; sleep \$i; done |$xclate_path &
	for i in 1 2 1 2 0; do echo \$i; sleep \$i; done |$xclate_path &
	wait" >$T1
set _ `cat <$T1`
shift
[ "1 2 1 2 0 1 2 1 2 0" = "$*" ]
# multiple output on the same line in parallel
$xclate_path -m ksh -c "exec </dev/null
	$xclate_path p ksh -c 'print \"foo\\c\"; sleep 1; print \"bar\\c\"' &
	$xclate_path q ksh -c 'print \"foo\\c\"; sleep 1; print \"bar\\c\"' &
	wait" >$T1
set _ `cat <$T1`
shift
[ "foobarfoobar" = "$*" ]


# Try headers and horizontal rules
$xclate_path -m $xclate_path -T "top %x %s" -H "bottom %s %x" name  echo OK </dev/null >$T1
cat <<\! >$T2
top name 0
OK
bottom 0 name
!
cmp $T1 $T2
# in filter mode
echo OK | $xclate_path -m $xclate_path -T "top %x %s" -H "bottom %s %x" name  >$T1
cmp $T1 $T2

# try variable expansion in header/footer
X='boo%yeah'
export X
$xclate_path -m $xclate_path -T "top %{T2}" -H "bottom %{X}" var echo okay </dev/null >$T1
cat <<! >$T2
top $T2
okay
bottom $X
!
cmp $T1 $T2
# in filter mode
echo okay |$xclate_path -m $xclate_path -T "top %{T2}" -H "bottom %{X}" var >$T1
cmp $T1 $T2

# See if the  master will pass headers to the client
X='Boo%rahh'
export X
$xclate_path -m -T "top %{T2}" -H "bottom %{X}" $xclate_path var echo okay </dev/null >$T1
cat <<! >$T2
top $T2
okay
bottom $X
!
cmp $T1 $T2
# in filter mode
echo okay |$xclate_path -m -T "top %{T2}" -H "bottom %{X}" $xclate_path var >$T1
cmp $T1 $T2

# N notify message this file, process, or fd for each finished section
> $T2
$xclate_path -m -N $T2 ksh -c "exec </dev/null
	$xclate_path not true
	$xclate_path so true
	$xclate_path bad false; exit 0"
set _ `tr '\000' ' ' < $T2`
shift
[ $# -eq 3 ] && [ $1 = not ] && [ $2 = so ] && [ $3 = bad ]

# -N "|proc"
# r        report exit codes in the notify stream
> $T2
$xclate_path -mr -N "|tr '\000' ' ' >$T2" ksh -c "exec </dev/null
	$xclate_path not true
	$xclate_path so false
	$xclate_path bad true"
set _ `cat  <$T2`
shift
[ $# -eq 3 ] && [ $1 = 0,not ] && [ $2 = 1,so ] && [ $3 = 0,bad ]

# -N '>&fd'
# should close the fd in it cmd process
exec 6>$T2
echo "begin" 1>&6
$xclate_path -m -N '>&5' 5>&6 2>/dev/null </dev/null $xclate_path bravo \
	ksh -c "echo wrong 1>&5; exit 0"
echo "end" 1>&6
exec 6>&-
cat <<\! >$T1
begin
bravo
end
!
tr '\000' '\n' <$T2 |diff - $T1

# W widow  redirect widowed output to this file, process, or fd (append mode)
echo append >$T2
$xclate_path -m -W ">>$T2"  ksh -c "exec </dev/null
	echo widow1
	echo collate2 | $xclate_path --
	echo widow2
	" >$T1
set _ `cat $T1 $T2`
shift
[ "$*" = "collate2 append widow1 widow2" ]

# O output redirect collated output to this file, process, or fd
X='boo%yeah'
$xclate_path -m -O "|tr '[A-Z]' '[a-z]' >$T1" ksh -c "
	read X _trash
	echo \$X | (sleep 2 ; $xclate_path delay ) &
	echo '$X' | $xclate_path first
	$xclate_path 2-3  sed -e 's/LINE/word/'
	wait" <<\!
FooBee Bletch
LINE2
wOw
!
set _ `cat $T1`
shift
[ "$*" = "boo%yeah word2 wow foobee" ]

>$T1
>$T2
# -depth   specify collation with an outer copy of xclate
$xclate_path -m -O "$T1" ksh -c "
	( sleep 3 ; $xclate_path IV echo 4) &
	$xclate_path -m -O '$T2' ksh -c \"
		echo 1 | $xclate_path -1 one
		$xclate_path -0 three echo 3
		\" </dev/null
	$xclate_path two echo 2 </etc/passwd
	$xclate_path dup cat <$T2
	wait
	"
set _ `cat $T1`
shift
[ "$*" = "1 2 3 4" ]

# u unix   specify a forced unix domain end-point name
$xclate_path -m -O "$T1" ksh -c "
	$xclate_path -m -O $T2 ksh -c \"
		print 'push\\\\c' | $xclate_path roll
		print 'punch through' | $xclate_path -u \\\${xcl_1} rock
		print ' back' | $xclate_path -u \\\${xcl_2} up\""
set _ `cat $T1 $T2`
shift
[ "punch through push back" = "$*" ]

# test -r with some competition in the mix, emulate xapply the hard way
$xclate_path -mr -N "|tr '\\000' '\\n' |sort >$T1" \
	ksh -c 'exec </dev/null
		$xclate_path 0 sh -c "sleep 1; echo 0; true" &
		$xclate_path 1 sh -c "sleep 1; echo 1; false" &
		$xclate_path 2 sh -c "sleep 1; echo 2; exec true" &
		$xclate_path 3 sh -c "sleep 1; echo 3; exec false" &
		$xclate_path 4 sh -c "sleep 1; echo 4; exit 5" &
		$xclate_path 5 sh -c "sleep 1; echo 5; exec sleep 4" &
		wait' |sort >$T2
set _ `cat $T1`
shift
[ "0,0 0,2 0,5 1,1 1,3 5,4" = "$*" ]
set _ `cat $T2`
shift
[ "0 1 2 3 4 5" = "$*" ]

# We have a clever use of xcl_0, -Q, and : mode to check setting
# $xcl_0 as a fake level 0 clues xclate into using the directory
# part of the path as its nesting place.  You must put a socket
# name on the end (e.g. /tmp/nesting/fake-name). Pause for spin-up.
EDIR=$TMPDIR/xreg$((RANDOM%10000)),$$
while ! mkdir -m 0700 $EDIR ; do
	EDIR=$TMPDIR/xreg$((RANDOM%10000)),$((RANDOM%1964))
done
date |
>$T1
>$T2
xcl_0=$EDIR/bogus $xclate_path -m -O $T1 -W $T2 : &
CPID=$!
sleep 1
# We should have a socket in $EDIR/1 now
[ -S $EDIR/1 ]

# Force text into the widow diversion (the number five)
$xclate_path -u $EDIR/1 -w weeper expr 2 + 3 </dev/null
[ `cat $T2` = 5 ]

# Put our $PWD in $T1, and tell the process to exit, so the socket should
# be gone as well.  Then clean up the directories we made
$xclate_path -Q -u $EDIR/1 test /bin/pwd </dev/null
[ `cat $T1` = `/bin/pwd` ]
[ ! -e $EDIR/1 ]
rmdir $EDIR
unset xcl_0 || true

# make sure we remove the xclXXXXXX dirs
unset ChecK || true
eval `$xclate_path -m env 2>&1 | sed -n -e "s/^xcl_1=/ChecK=/p"`
[ ! -d  ${ChecK%/*} ]

# test -i, for files and fd's, we don't test sockets (only because
# we might not have an nc with -U installed)
echo "Red" >$T1
echo "Blue" >$T2
[ _Blue = _`$xclate_path -m <$T2 $xclate_path this sh -c 'read x; echo $x'` ]
[ _Red = _`$xclate_path -m -i "<$T1" <$T2 $xclate_path this sh -c 'read x; echo $x'` ]
[ _Red = _`$xclate_path -m -i "<&8" <$T2 $xclate_path this sh -c 'read x; echo $x' 8<$T1` ]
[ _Pink = _`$xclate_path -m -i "|echo Pink;echo Blue" <$T2 $xclate_path this sh -c 'read x; echo $x' 8<$T1` ]
[ _Green = _`$xclate_path -m -i "|echo Pink;echo Green" <$T2 $xclate_path this sh -c 'read x; read x; echo $x' 8<$T1` ]


# test -I, for a kick
echo "Blue" >$T1
echo "Red" >$T2
[ _Blue = _`$xclate_path -m <$T1 -i $T2 $xclate_path -I this sh -c 'read x; echo $x'` ]
[ _Red = _`$xclate_path -m <$T2 -i $T1 $xclate_path -I this sh -c 'read x; echo $x'` ]
[ _Blue = _`cat $T1|$xclate_path -m -i /dev/null $xclate_path -I this sh -c 'read x; echo $x'` ]
[ _Red = _`cat $T2|$xclate_path -m -i /dev/null $xclate_path -I this sh -c 'read x; echo $x'` ]

# test -E, we must turn off -x in most shells, as it pollutes stderr
(set +x; $xclate_path -m 2>$T2 sh -c "exec 2>$T1; $xclate_path    err sh -c 'echo Emu >&2'")</dev/null
[ _Emu = _`cat $T1` -a _ = _`cat $T2` ]
(set +x; $xclate_path -m 2>$T2 sh -c "exec 2>$T1; $xclate_path -E err sh -c 'echo Emu >&2'")</dev/null
[ _ = _`cat $T1` -a _Emu = _`cat $T2` ]

# test -D (assumes we have a /usr directory)
[ _/usr = _`cd /usr && $xclate_path -m sh -c 'cd /; exec $xclate_path -D this pwd' </dev/null` ]


# Not really tested here
# -Y, sshw does, and we don't have 2 tty devices (use maybe script -- Nahh)
# -L cap    cap on memory based output buffer
exit 0
