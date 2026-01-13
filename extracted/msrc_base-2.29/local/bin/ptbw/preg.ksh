#!/usr/bin/env ksh
# $Id: preg.ksh,v 1.11 2008/09/11 22:25:15 ksb Exp $
# Regression test for ptbw version 1.40 or better			(ksb)
# The output should look sane and exit 0, output may differ a little
# based on some difference in CPUs and resources.
# This should take about 4-5 seconds to run.
# We depend on /usr/bin/ programs mostly (and xclate if installed)
#
# We don't check that we could force-set a $ptbw_0 and use it to track	XXX
# off-the-stack references.

SHELL=`whence sh`
export SHELL
unset ptbw_link ptbw_0 ptbw_1 ptbw_2 ptbw_3 ptbw_d ptbw_list
unset xcl_link xcl_0 xcl_1 xcl_2 xcl_3 xcl_d


: ${TMPDIR:=/tmp}
while T1=$TMPDIR/prt$$,e$((RANDOM%1000)) ; do
	[ -f $T1 -o -e $T1.w ] || break
done
while T2=$TMPDIR/prt$$,o$((RANDOM%1000)) ; do
	[ -f $T2 -o -e $T2.w ] || break
done
while T3=$TMPDIR/prt$$,n$((RANDOM%1000)) ; do
	[ -f $T3 ] || break
done
trap 'rm -f $T1 $T2 $T3 $T1.w $T2.w' EXIT

cat <<\! >$T1
0
1
2
3
4
5
6
7
8
9
!

# We need a file that doesn't exist for some tests
rm -f $T2

ptbw_path=${1:-ptbw}
# Check for nonexistant files and empty (or short) token lists
$ptbw_path -m -t $T2  true 2>/dev/null
[ $? -eq 66 ] || exit 1
touch $T2
$ptbw_path -m -t $T2  true 2>/dev/null
[ $? -eq 65 ] || exit 1
echo "OK" >>$T2
$ptbw_path -m -t $T2  true 2>/dev/null
[ $? -eq 0 ] || exit 1
$ptbw_path -m -t $T2 -R 3  true 2>/dev/null
[ $? -eq 65 ] || exit 1

cat <<\! >$T2
lonely
bravo
three
fourth
pentagon
sense
sins
pieces
IX
decade
!

# $1 is a file of ptbw tokens, $2 is a file of those aranged in lines	(ksb)
# return 0 if all the tokens in the lines are lines in the ptbw file
function AllIn {
	RET=0
	tr ' \007' '\012\012' < $2 |
	while read word ; do
		grep -F -w "$word" $1 >/dev/null && continue
		RET=1
		break
	done
	return $RET
}

# Finish short token list checks, note that 3*7 is > 10
$ptbw_path -m -t $T2 -R 3 -J 7  true 2>/dev/null
[ $? -eq 0 ] || exit 2

# All the remaining tests must exit 0
set -e
$ptbw_path -V >/dev/null
$ptbw_path -h >/dev/null
# If we hang here we have the bug in ptbw 1.53 where we try to get
# tokens from ourself rather than the enclosing diversion. -- ksb
$ptbw_path -m -t $T1  $ptbw_path -t - $ptbw_path -V |fgrep '[target]' >/dev/null

# Start some bg brokers to poke at
$ptbw_path -mt $T1 -N $T1.w : &
PT1=$!
$ptbw_path -mt $T2 -N $T2.w : &
PT2=$!
sleep 1

# Check that we can get the source file name, even nested
[ _$T1 = _$($ptbw_path -t $T1.w |sed -n -e 's/.*from file: \(.*\)/\1/p') ] || exit 3
[ _$T2 = _$($ptbw_path -t $T2.w |sed -n -e 's/.*from file: \(.*\)/\1/p') ] || exit 3
[ _$T2 = _$($ptbw_path -mt $T2.w -R3 $ptbw_path |sed -n -e 's/.*from.*file[: ]*\(.*\)/\1/p') ] || exit 3
[ _iota = _$($ptbw_path -m -R3 $ptbw_path |sed -n -e 's/.*from.*function[: ]*\(.*\)/\1/p') ] || exit 3

# Check basic -A and -R function, as well as -R 0
for i in 1 2 3 4 5 6 7 8 9 ; do
	$ptbw_path -t $T1.w -R $i -A echo
done | tr -d '[ 0-9\012] ' >$T3
[ -s $T3 ] && exit 10
[ 0 -eq `$ptbw_path -t $T1.w -R 0 -A echo | wc -w` ] || exit 11

# Check token allocation (as best we can)
(
for i in 1 1 2 2 3 1 2 1 2 1 3 10 3 3 1 ; do
	$ptbw_path -t $T2.w -R $i $SHELL -c "echo \$ptbw_list"
done
) >$T3
AllIn $T2 $T3 || exit 12
# Work a little harder in parallel
(
	$ptbw_path -t $T2.w -R  4 $SHELL -c "echo \$ptbw_list; sleep 1" &
	$ptbw_path -t $T2.w -R  4 $SHELL -c "echo \$ptbw_list; sleep 1" &
	$ptbw_path -t $T2.w -R  4 $SHELL -c "echo \$ptbw_list; sleep 1" &
	$ptbw_path -t $T2.w -R 10 $SHELL -c "echo \$ptbw_list" &
	$ptbw_path -t $T2.w -R  2 $SHELL -c "sleep 1; echo \$ptbw_list" &
	$ptbw_path -t $T2.w -R  2 $SHELL -c "sleep 1; echo \$ptbw_list" &
	$ptbw_path -t $T2.w -R  2 $SHELL -c "sleep 1; echo \$ptbw_list" &
	$ptbw_path -t $T2.w -R  2 $SHELL -c "sleep 1; echo \$ptbw_list" &
	$ptbw_path -t $T2.w -R  2 $SHELL -c "sleep 1; echo \$ptbw_list" &
	wait
) > $T3
AllIn $T2 $T3 || exit 12

# Start a cleanup on our bg services (tell them to exit)
kill -TERM $PT1 $PT2

# Check -d
eval `$ptbw_path -m -R 10 -J 1 $ptbw_path -md -t $T2 \
	$ptbw_path -R1 $SHELL -c 'echo link=$ptbw_link alloc=$ptbw_list'`
[ $link -eq 1  -a  $alloc -eq 0 ] || exit 13
eval `$ptbw_path -md -t $T2  $SHELL -c 'echo hidden=$ptbw_d'`
[ -z "$hidden" ] && exit 13
eval `ptbw_d=$T2.x $ptbw_path -m -t $T2 \
	$SHELL -c 'echo link=$ptbw_link hidden=$ptbw_d'`
[ $link -eq 1  -a _$T2.x = _"$hidden" ] || exit 13


# Check -q really squelches the limit warning
$ptbw_path -m -q -t $T2 -R3 -J7  true 2>$T3
[ -s $T3 ] && exit 14

# Check cleanup for the sockets we build
wait
[ -e $T1.w  -o  -e $T2.w ] && exit 15

# Check for colon mode and -Q
$ptbw_path -mt $T2 -N $T2.w : &
sleep 1
$ptbw_path -t $T2.w -Q -A echo >$T3
wait
AllIn $T2 $T3 || exit 16

# Check -depth option
$ptbw_path -mt $T2 -N $T2.w  \
	$ptbw_path -mt $T1 -N $T1.w $ptbw_path -1 -AR 2 echo >$T3
AllIn $T2 $T3 || exit 17

# Check that we can hide inside xclate, if installed (could look for 1 socket)
xclate_path=`set +e; whence xclate 2>/dev/null`
if [ -n "$xclate_path" ] ; then
	[ `$xclate_path -m $ptbw_path -m ksh -c 'echo \$(dirname $ptbw_1) = \$(dirname $xcl_1) |xclate test'` ] || exit 20
fi

# Check for comment in tags files working.  For a long time they didn't (1.59).
cat >$T2<<\!
#comment
token1
token2
!
$ptbw_path -mt $T2 -N $T2.w  \
	$ptbw_path -AR 1 echo >$T3
[ _token1 = _`cat $T3` ] || exit 21

# check exit code
$ptbw_path -m true
[ $? -eq 0 ] || exit 30
set +e
$ptbw_path -m false
[ $? -ne 0 ] || exit 30
$ptbw_path -m sh -c "exit 45"
[ $? -eq 45 ] || exit 30
set -e

exit 0
