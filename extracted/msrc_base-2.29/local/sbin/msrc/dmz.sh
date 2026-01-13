#!/bin/ksh
# $Id: dmz.sh,v 1.14 2010/07/14 17:38:22 ksb Exp $
# A filter to process command line arguments like smsrc for msrc --ksb
# Install this in a common directory all Admins search, name it for
# the desired default hxmd configuration file.  For example "dmz"
# implies -C "dmz.cf" and maybe the -Z file "dmz.zf".
#
# Sample usage from smsrc:
#	op smsrc -m adm1,adm2,adm3 install
# New usage for this script:
#	dmz adm1,adm2,adm3  make install
# $Posse(*): ${efmd:-efmd} -C%f ${EXCF:+"-X$EXCF"} ${ENVCF:+"-Z$ENVCF"} -E 'yes=SERVICES(%s)' -L
progname=`basename $0`
usage="$progname: usage [-C config] [-X ex-cfg] [-Z env] -S services | hosts [msrc-opts] [utility]"
# the -u option is now processed by msrc itself, we still take it because
# it might be on our side (left of) the host list. -- ksb

# How to slide a single letter option off the beginning of a bundle
# -barf -> -arf
slide='P=$1; shift; set _ -`expr _"$P" : _'\''-.\(.*\)'\''` ${1+"$@"}; shift'
param='if [ $# -lt 2 ]; then echo "$progname: missing value for $1" 1>&2 ; exit 1; fi'

# Default values for all the flags, or leave unset for a ${flag-value} form
# Remove the comment on the ENVCF set below if you use that feature, this is
# the line that makes the "maybe -Z..." above true.
#: ${ENVCF:=${progname}.zf}
: ${DEFCF:=${progname}.cf}
: ${ELOGIN=""}

# Set LOCAL_PRIV to "op" or "sudo" to always priv-up remotely, note that this
# limits the commands you can run to those that have escalation rules.
# Set this to "--" to remove the spell for any command you can run as
# yourself: the default is clearly site policy. -- ksb
: ${LOCAL_PRIV=""}
USESvcs=false

# Get the options from the command line (+ any variables)
while [ $# -gt 0 ]
do
	case "$1" in
	-C)
		eval "$param"
		DEFCF=$2
		shift ; shift
		;;
	-C*)
		DEFCF=`expr _"$1" : _'-.\(.*\)'`
		shift
		;;
	-S)
		USESvcs=true
		shift
		;;
	-S*)
		USESvcs=true
		eval "$slide"
		;;
	-u) # depricated since msrc takes it now
		eval "$param"
		ELOGIN=$2
		shift ; shift
		;;
	-u*)
		ELOGIN=`expr _"$1" : _'-.\(.*\)'`
		shift
		;;
	-X)
		eval "$param"
		EXCF=${EXCF+"$EXCF:"}$2
		shift ; shift
		;;
	-X*)
		EXCF=${EXCF+"$EXCF:"}`expr _"$1" : _'-.\(.*\)'`
		shift
		;;
	-Z)
		eval "$param"
		ENVCF=${ENVCF+"$ENVCF:"}$2
		shift ; shift
		;;
	-Z*)
		ENVCF=${ENVCF+"$ENVCF:"}`expr _"$1" : _'-.\(.*\)'`
		shift
		;;
	--)
		shift
		break
		;;
	-h|-h*)
		cat <<HERE
$usage
$progname: usage -h
$progname: usage -V
C config  specify a configuation file${DEFCF:+" (default $DEFCF)"}
h         print this help message
S service specify services to pick hosts separated by commas
V         show version information only
X ex-cfg  specify an extra configuration${EXCF:+" (default $EXCF)"}
Z env     specify an environment configuration${ENVCF:+" (default $ENVCF)"}
hosts     target hosts separated with commas
msrc-opts any other option to msrc, hxmd, xapply, xclate, or ptbw
utility   options to msrc and the utility to execute
HERE
		exit 0
		;;
	-V*)
		echo $progname: '$Id: dmz.sh,v 1.14 2010/07/14 17:38:22 ksb Exp $'
		exec msrc -V
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

if [ $# -lt 1 ] ; then
	echo "$usage" 1>&2
	exit 64
fi

if $USESvcs ; then
	export EXCF ENVCF	# For any mk templates that need them
	TSPACE=$(mktemp -t posse$$) || exit $?
	trap "rm -rf $TSPACE" EXIT INT TERM
	E=$1
	for cFile in $(echo $DEFCF | tr ',:' '  ')
	do
		cFile=$(efmd -C$cFile -T HXMD_OPT_C dnl)
		for sName in $(echo $1 | tr ',:' '  ')
		do
			mk -st$0:/usr/local/sbin/dmz${ENVCF:+":$ENVCF"}${EXCF:+":$EXCF"} -mPosse -d$sName $cFile
		done
	done >$TSPACE
	shift
	msrc ${EXCF:+"-X$EXCF"} ${ENVCF:+"-Z$ENVCF"} ${DEFCF:+"-C$DEFCF"} ${ELOGIN:+"-DENTRY_LOGIN=$ELOGIN"} -G dnl -Y "include(\`$TSPACE')" -N "echo 'No hosts matched a service list \"$E\" from $DEFCF'%0" $LOCAL_PRIV "$@"
	exit
fi
# Convert the host list into something m4 can deal with (viz. _A_B_C_).
HTEMPu=`echo _${1}_ | tr -cs '\-.a-zA-Z0-9' '_'`
shift
exec msrc ${EXCF:+"-X$EXCF"} ${ENVCF:+"-Z$ENVCF"} ${DEFCF:+"-C$DEFCF"} ${ELOGIN:+"-DENTRY_LOGIN=$ELOGIN"} -E "-1!=index(\`$HTEMPu',_\`'SHORTHOST\`'_)" -N "echo 'No hosts matched from $DEFCF'%0" $LOCAL_PRIV "$@"
