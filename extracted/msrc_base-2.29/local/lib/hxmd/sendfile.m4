dnl $Id: sendfile.m4,v 1.6 2009/09/24 17:15:09 ksb Exp $
dnl
dnl These macros are for msrc's provision logic and cannot be used outside
dnl of the INIT_CMD, PRE_CMD, or POST_CMD hooks (without a fake context).
dnl
dnl SendFile(source,dest) msrc provision in either local or remote mode (ksb)
pushdef(`SendFile',
`ifdef(`RDIST_PATH',`',`errprint(`msrc: sendfile.m4: RDIST_PATH: definition missing, included outside of provision script?')m4exit(70)')dnl
ifelse($1,`',`errprint(`msrc: sendfile.m4: no file specification')m4exit(66)')dnl
`# sending $1
RNAME='$2`
: ${RNAME:=$(basename '$1`)}
if [ -z "${5}" -o -z "${3}" -o -z "${1}" ] ; then
	echo "msrc: sendfile: no INTO or MODE set in \${5} and \${3}" 1>&2
	exit 76			 # PROTOCOL
fi
[ -z "'$1`" ] || case _${3} in
_local)
	cp '$1` ${1}/$RNAME ;;
_remote)
	'RDIST_PATH ifdef(`RSH_PATH',`-P`'RSH_PATH') ifdef(`RDISTD_PATH',`-p`'RDISTD_PATH') \
		`-c' $1 ifdef(`ENTRY_LOGIN',`ENTRY_LOGIN@')HOST:${5}/$RNAME` ;;
*)
	echo "msrc: sendfile: ${3}: unknown MODE" 1>&2
	exit 78 ;;		# CONFIG
esac
'')dnl
dnl Arange for the msrc provision script to send arbitrary files to the	(ksb)
dnl target cache directory.
dnl
dnl SendFiles(source1,dest1, source2,dest2, ...)  destN may be empty
pushdef(`SendFiles',`ifelse($1,`',`',
`SendFile($1,$2)dnl
SendFiles(shift(shift($*)))')')dnl
dnl
dnl Undo our inclusion, like we were never here				(ksb)
dnl
pushdef(`SendFilePop',`popdef(`SendFilePop')popdef(`SendFiles')popdef(`SendFile')')dnl
