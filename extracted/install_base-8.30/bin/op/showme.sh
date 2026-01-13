#!/bin/sh
# $Id: showme.sh,v 2.29 2008/12/31 20:51:12 ksb Exp $
# $Doc: sed -e 's/&/A''MPamp;/g' -e 's/</\\&lt;/g' -e 's/>/\\&gt;/g' <%f| sed -e 's/[A][M][P]/\\&/g'
(
if [ $# -eq 0 ] ; then
	echo "Process #$$ was executed as \"$0\", with no arguments"
else
	echo "Process #$$ was executed as \"$0\", with arguments of"
	for A
	do
		echo "$A" |sed -e 's/\([\\"$`]\)/\\\1/g' -e 's/^/  "/;s/$/"/'
	done
fi
echo " as login" `id -rnu`", effective" `id -nu`" [uid" \
	`id -ru`", "`id -u`"]"
echo " with group" `id -rng`", effective" `id -ng`" [gid" \
	`id -rg`", "`id -g`"]"
echo " supplementary groups:" `id -nG` "[`id -G`]"
echo ' $PWD='`pwd`
echo ' umask '`umask`
renice 0 $$ 2>&1 |sed -e 's/old/nice/' -e 's/,.*//'
env
) |${PAGER:-more}
exit 0
