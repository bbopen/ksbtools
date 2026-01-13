#!/usr/bin/env ksh
# $Id: xreg.ksh,v 1.12 2008/04/02 15:32:20 ksb Exp $
# Regression test xapply V3.26 (ksb)
# The output should look sane and exit 0, same output on all OS types.
# It takes about 20 seconds to run.					(ksb)
SHELL=`whence sh`
export SHELL

: ${TMPDIR:=/tmp}
while T1=$TMPDIR/xat$$,e$((RANDOM%1000)) ; do
	[ -f $T1 ] || break
done
while T2=$TMPDIR/xat$$,o$((RANDOM%1000)) ; do
	[ -f $T2 ] || break
done
trap 'rm $T1 $T2' EXIT

cat <<\! >$T1
I
II
III
IV
V
!
cat <<\! >$T2
one
two
three
four
five
!

# Clean up any ptbw in our environment
unset ptbw_link ptbw_list ptbw_0 ptbw_1 ptbw_2 ptbw_d

set -e
xapply -V >/dev/null
xapply -h >/dev/null
#   default param processesing
[ _"`xapply -n '1' ''`_" ==  _"`xapply -n '' '1'` _" ]
[ _"`xapply -n '2 3' ''`_" ==  _"`xapply -n '2' '3'` _" ]
[ _"`xapply -n '4 5' ''`_" ==  _"`xapply -n '' '4 5'` _" ]

# -count      number of arguments passed to each command
[ "_A B_" ==  _"`xapply -n -2 '' 'A' 'B'`_" ]
[ "_C D E_" ==  _"`xapply -n -3 '' 'C' 'D' 'E'`_" ]
[ "_F G H I_" ==  _"`xapply -n -4 '' 'F' 'G' 'H' 'I'`_" ]

# f           arguments are read indirectly one per line rom files
# z           read find-print0 output as input files
[ 2 -eq `find /dev/null /dev/null -print |
	xapply -fn '' - | wc -l` ]
[ 2 -eq `find /dev/null /dev/null -print0 |
	xapply -fzn '' - | wc -l` ]
[ 21 -eq `find /dev/null /dev/null -print |
	xapply -fzn '' - | wc -c` ]

# A           append the tokens as shell parameters
[ "_A 0_" == _"`xapply -A 'echo %1 $1' A`_" ]
# S shell     the shell to run tasks under
for XSH in csh tcsh ksh sh pdksh ash
do
	WSH=`whence $XSH` || continue
	[ "_A 0_" == _"`xapply -A -S$WSH 'echo %1 $1' A`_" ]
done
# and it works for perl too!
if WSH=`whence perl` ; then
	[ "_P0_" == _"`xapply -A -S$WSH 'print "%1",$ARGV[0],"\n";' P`_" ]
fi

# e var=dicer set var to the expansion of dicer for each process
[ "_ExParTay_" == _"`xapply -e E=Ex -e D=Tay 'echo ${E}%1' 'Par$D'`_" ]
[ "_ExParTay Par_" == _"`xapply -e E=Ex -e D=Tay -e A 'echo $E$A$D' 'Par'`_" ]

# dicer 0 param
[ "__" == _"`xapply -n '%0' /dev/null`"_ ]
[ "_%_" == _"`xapply -n '%0%%' /dev/null`"_ ]

# dicer 1 param
[ "_/dev/null_" == _"`xapply -n '%1' /dev/null`"_ ]
[ "_/dev/null_" == _"`xapply -n '%{1}' /dev/null`"_ ]
[ "_/dev/null_" == _"`xapply -n '%[1]' /dev/null`"_ ]
[ "_/dev/null_" == _"`xapply -n '%$' /dev/null`"_ ]
[ "_/dev/null_" == _"`xapply -n '%{$}' /dev/null`"_ ]
[ "_/dev/null_" == _"`xapply -n '%[$]' /dev/null`"_ ]
[ "_/dev/null_" == _"`xapply -n '%*' /dev/null`"_ ]
[ "_/dev/null_" == _"`xapply -n '%{*}' /dev/null`"_ ]
[ "_/dev/null_" == _"`xapply -n '%[*]' /dev/null`"_ ]
[ "__" == _"`xapply -n '%[1/1]' /dev/null`"_ ]
[ "_dev_" == _"`xapply -n '%[1/2]' /dev/null`_" ]
[ "_null_" == _"`xapply -n '%[1/3]' /dev/null`_" ]
[ "_null_" == _"`xapply -n '%[1/$]' /dev/null`_" ]

# dicer 2 params
[ "_dev_" == _"`xapply -2 -n '%1' 'dev' 'null'`"_ ]
[ "_null_" == _"`xapply -2 -n '%2' 'dev' 'null'`"_ ]
[ "_dev null_" == _"`xapply -2 -n '%*' 'dev' 'null'`_" ]
[ "_dev null_" == _"`xapply -2 -n '' 'dev' 'null'`_" ]
[ "_/dev/null_" == _"`xapply -2 -n '/%1/%2' 'dev' 'null'`_" ]

# dicer double-down
[ "_5271009_" = _`xapply -n "%[1:5,3]" 'ksb:*:11517:2060:KS Braunsdorf,Wizard,5271009:/home/sac1/ksb:/bin/ksh'`_ ]
[ "_9,13:10,22_" = _`xapply -2 -n "%[1,-$,-1]:%[2,-1,-3,-1]" '3,9,13,15' '6,8,10,16,22'`_ ]

# dicer quote ] and blanks, and negative subfields
[ "_k_" = _"`xapply -n '%[1 3]' 'a  b k	s'`_" ]
[ "_s_" = _"`xapply -n '%[1\	2]' 'a  b k	s'`_" ]
[ "_b_" = _"`xapply -n '%[1\ 3]' 'a  b k	s'`_" ]
[ "_[theta_" = _"`xapply -n '%[1\]2,1]' '[5,6][theta,omega]'`_" ]
[ "_]_" = _"`xapply -n '%[1\[-3\6$]' '[5,6][theta,omega]'`_" ]

# %+ shift params to a new cmd, more negative subfields
[ "_5_" = `xapply -2 'echo "_%+_"' "%[1,-$]" "5,6"` ]
[ "_<5,7>_" = `xapply -3 'echo "_%+_"' "<%+>" "%[1,-2]" "5,6,7"` ]
# long standing bug in %u, 2 is the wrong answer here --ksb
[ 0 -eq `echo "echo %u" | xapply -f '%+' /dev/null /dev/null - /dev/null` ]
# another one, sigh.
set _ `(echo a; echo b; echo c) | xapply -nf '%u%1' -`
shift
[ _$1 = _0a ] && [ _$2 = _1b ] && [ _$3 = _2c ]

# %f under -f
[ "_${T1} ${T2}_" = _"`xapply -f -2 '%0echo %f1 %f2' $T1 $T2 |tail -1`"_ ]
[ "_${T1} ${T2}_" = _"`xapply -f -2 '%0echo %f*' $T1 $T2 |tail -1`"_ ]
[ "_${T1} ${T2} V five_" = _"`xapply -f -2 'echo %f1 %f2' $T1 $T2 |tail -1`"_ ]
[ "_${T1} ${T2} V five_" = _"`xapply -f -2 'echo %f*' $T1 $T2 |tail -1`"_ ]

# a c         change the escape character
[ "_/dev/null_" == _"`xapply -a @ -2 -n '/@1/@2' 'dev' 'null'`_" ]
[ "_dev null_" == _"`xapply -a~ -2 -n '~*' 'dev' 'null'`_" ]
[ "_dev!null_" == _"`xapply -a! -2 -n '![1]!!!{2}' 'dev' 'null'`_" ]
[ "_dev%null_" == _"`xapply -a! -2 -n '![1]%!{2}' 'dev' 'null'`_" ]

# m           use xclate to manage output streams
# Pjobs       number of tasks to run in parallel
# xclate must be installed from here one
WSH=`whence ksh`
[ "_foofoobarbar_" == _"`xapply -S$WSH -P2 'print "foo\c"; sleep 1; print "%1\c"' bar bar`"_ ]
[ "_foobarfoobar_" == _"`xapply -m -S$WSH -P2 'print "foo\c"; sleep 1; print "%1\c"' bar bar`"_ ]

# u           force the xclate xid to be the expansion of %u, see -N below
set _ `XCLATE_1='-T %x' xapply -m 'expr 7 + %1' 3`
shift
[ 3 -eq $1 ] && [ 10 -eq $2 ]
set _ `XCLATE_1='-T %x' xapply -mu 'expr 7 + %1' 3`
shift
[ 0 -eq $1 ] && [ 10 -eq $2 ]

# i input     change stdin for the child processes
set _ `xapply -f -i $T1 'read L; echo $L %1' $T2 | wc`
shift
[ $1 -eq 5 ] && [ $2 -eq 10 ] && [ $3 -eq 38 ]

# p pad       fill short files with this token
set _ `xapply -2 -f -pthing 'echo %1 %2' $T2 /dev/null | wc`
shift
[ $1 -eq 5 ] && [ $2 -eq 10 ] && [ $3 -eq 54 ]

# ptbw must be installed from the point on
# t tags      list of target resources for %t, start a ptbw
# we can only get the first token (no -J/-P)
set _ `xapply -f -t $T1 'echo %t1 %1' $T2 | wc`
shift
[ $1 -eq 5 ] && [ $2 -eq 10 ] && [ $3 -eq 34 ]

# J tokens    total number of resources required, start a ptbw
# R req       number of resource allocated to each task
# test tableau we build with ptbw
set _ `xapply -fm -J5 -R3 -P5 '%0echo %u %t*' $T1 | sort -n | cut -c 2-`
shift
[ "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14" = "$*" ]
# test that we can get 5 tokens from the T1 file (force -P/-J to be all 5)
set _ `xapply -f -t $T1 -P5 -J5 'echo %t1 %1' $T2 | wc`
shift
[ $1 -eq 5 ] && [ $2 -eq 10 ] && [ $3 -eq 38 ]

# check with an enclosing ptbw (we didn't start)
# -t - and -t /path/to/socket
set _ `ptbw -m -t $T1 -R2 --  sh -c '
	ptbw -A  sh -c "echo \\\$1; sleep 3" _ &
	xapply -AR2 -t - "%0echo \"%t1 %t2\"; sleep 3" "" &
	xapply -AR2 -t $ptbw_1 "%0echo \"%t*\"; sleep 3" "" &
	wait' | tr ' ' '\012' | sort `
shift
[ "I II III IV V" = "$*" ]

# %ft file name for token list
set _ `xapply -t $T1 'echo %ft' BCPL`
shift
[ $1 = "$T1" ] && [ $2 = "BCPL" ]
set _ `xapply -R 2 -J 3 'echo %ft' APL`
shift
[ $1 = "iota" ] && [ $2 = "APL" ]

# %ft through 3 ptbw's (nothing but net)
[ "_${T1}_" = _"`xapply -t $T1 'echo %ft' ''`_" ]
[ "_${T1}_" = _"`ptbw -m -J1 -R4 -t $T1 \
	ptbw -m -J2 -R2 -t - \
	xapply -R2 -t - 'echo %ft' ''`_" ]

# -N else    set zero-pass shell command
set _ `XCLATE_1='-T %x' xapply -m -N "echo zero" 'expr 7 + %1'`
shift
[ 0 -eq $1 ] && [ "00" = "$1" ]  && [ zero -eq $2 ]
set _ `xclate -mr -N '|tr "\000" "\n"' xapply -m -N 'exit 8' exit`
[ _8,00 -eq _$1 ]
set _ `xclate -mr -N '|tr "\000" "\n"' xapply -mu -N 'exit 8' exit`
[ _8,00 -eq _$1 ]

set _ `XCLATE_1='-T %x' xapply -mu -N "echo zero" 'expr 7 + %1'`
shift
[ 0 -eq $1 ] && [ "0" = "$1" ]  && [ zero -eq $2 ]

# check the Mixer
set _ `xapply 'echo %(1,$-~4)' $T1/file
	xapply 'echo %(1,($-~4)($-1))' $T1/file
	xapply 'echo %([1/-1],$-~4)' $T1/file
	xapply 'echo %([1/-1],($-~4)($-1))' $T1/file`
shift
[ _"elif file elif file"  = _"$*" ]

# Check -s, this could be hard as we can't really depend on "time" or "time -p"
if WSH=`whence perl` ; then
	GOTIME=$(perl -e 'print time();')
	xapply -msP2 '[ %u -eq 1 ] && echo hold; sleep %1' 1 9 1 1 1 1 1 1 1 1 >/dev/null
	DELTA=$(perl -e "print time() - $GOTIME;")
	# 9.13 seconds might round to 11 worst case, not 13 w/o -s
	[ $DELTA -le 11 ]
fi


exit 0
