# $Id: cool.sh,v 1.1 1993/10/04 19:24:30 ksb Exp $
for h in `distrib -IH`
do
	echo remsh $h -n kill -TERM `remsh $h -n ps -ae \| grep qmgr | sed -e 's/?.*//'`
done | tee res
