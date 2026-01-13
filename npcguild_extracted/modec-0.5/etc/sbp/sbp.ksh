#!/bin/ksh
# $Id: sbp.ksh,v 1.2 2000/04/15 17:22:29 ksb Exp $
# Synchronizes the backup partitions with the primary partitions.	(ksb)
# Builds a shell script on stdout; please verify first, then execute.
#
# This script was coded orinally by Greg Rivers (gcr) and ksb.
# This is a simplified version of Greg's work, to document what we
# are replacing.  The script version worked for years, but had some
# hard coded knowlege (like all primary and backup slices had to
# live on targets t0/t1 and had to be on the same controller).
#
# When we moved to hosts with more contollers we had to "upgrade" sbp,
# as a side effect the new one works on more platforms.

echo "#!/bin/sh"
date +"# built %c on `hostname`"
echo set -e

sed -e 's!/dsk/!/rdsk/!' </etc/vfstab |
awk '/^[^ 	]*[ 	]*[^ 	]*[ 	]*\/backup/{
  if ("-" != $2) { print $2, $3;} else { print $1, $3;}}' |
sort +1 -2 |
while read RAWDEV MOUNTPOINT
do
	case $MOUNTPOINT in
	/backup)	echo echo y \| newfs -i 4096 $RAWDEV	;;
	/backup/var)	echo echo y \| newfs -i 6144 $RAWDEV	;;
	*)		echo echo y \| newfs -i 16384 $RAWDEV	;;
	esac

	echo mount $MOUNTPOINT
	echo cd $MOUNTPOINT
	echo ufsdump 0f - /${MOUNTPOINT##/backup} \| ufsrestore rf -
	echo rm restoresymtable
done

echo "cat <<'!' >/backup/etc/vfstab"
sed -e 's/t0d/tXd/g' /etc/vfstab | sed -e 's/t1d/t0d/g' | sed -e 's/tXd/t1d/g'
echo !

echo cd /\; sync\; sync

awk '/^[^ 	]*[ 	]*[^ 	]*[ 	]*\/backup/{print $3}' /etc/vfstab |
sort -r |
while read MOUNTPOINT
do
	echo umount $MOUNTPOINT
done

exit 0
