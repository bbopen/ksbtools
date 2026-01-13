#!/bin/sh
# $Id: posse.sh,v 1.2 2010/08/08 20:47:48 ksb Exp $
# Posse script: usage service-list configs
progname=`basename $0`
usage="$progname: usage service-list [configs]"
slide='P=$1; shift; set _ -`expr _"$P" : _'\''-.\(.*\)'\''` ${1+"$@"}; shift'
param='if [ $# -lt 2 ]; then echo "$progname: missing value for $1" 1>&2 ; exit 1; fi'

# We must have some programs from /usr/local/{bin,sbin} (oue, mk, efmd),
# so we push those in before ".", if it is on the end.  I know that's
# strange, but you shouldn't include $PWD in your $PATH anyway.
case ":$PATH:" in
*:/usr/local/bin:*)
	;;
*)
	if expr "_$PATH" : '_.*:$' >/dev/null ; then
		PATH="$PATH/usr/local/bin:"
	else
		PATH="$PATH:/usr/local/bin"
	fi
	;;
esac
case ":$PATH:" in
*:/usr/local/sbin:*)
	;;
*)
	if expr "_$PATH" : '_.*:$' >/dev/null ; then
		PATH="$PATH/usr/local/sbin:"
	else
		PATH="$PATH:/usr/local/sbin"
	fi
	;;
esac

while [ $# -gt 0 ]
do
	case "$1" in
	--)
		shift
		break
		;;
	-h*)
		cat <<HERE
$usage
$progname: usage -h
$progname: usage -V
h             print this help message
V             show version information only
service-list  a list of the services to select separated by comma or colon
configs       hxmd-style configuration files
HERE
		exit 0
		;;
	-V*)
		echo $progname: '$Id: posse.sh,v 1.2 2010/08/08 20:47:48 ksb Exp $'
		exit 0
		;;
	-*)
		echo "$usage" 1>&2
		exit 64
		;;
	*)
		# No intermixed options & args for this program.
		break
		;;

	esac
done
sList=${1?"$usage"}
shift

# If you do not have "oue" installed we fall-back to perl.  You should
# install oue, it is your friend.
if which -s oue >/dev/null ; then
	Out_Filter="oue"
else
	Out_Filter="perl -e 'while (<>) { next if \$s{\$_}; \$s{\$_}=1; print \$_;}'"
fi

# We prefer to use efmd because it doesn't execute a ":" command for each key
# in every configuration file, which is silly and can take multiple seconds.
if which -s efmd >/dev/null ; then
	Abs_Filter="efmd -C \$cFile -T HXMD_OPT_C dnl"
else
	Abs_Filter="hxmd -C\$cFile -K 'echo HXMD_OPT_C' -Y \"undefine(\\\`Hack2gEt1')\" -G \"ifelse(defn(\\\`Hack2gEt1'),\\\`',\\\`HOST\\\`'define(\\\`Hack2gEt1',1)')\" :"
fi
set -e
MK=
for cFile
do
	eval "cPath=\$($Abs_Filter)"
	xapply "mk -smPosse -d%1 $cPath" $(echo $sList |tr ':,' '  ')
done |eval $Out_Filter
