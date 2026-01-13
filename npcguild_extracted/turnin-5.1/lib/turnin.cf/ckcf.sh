#!/bin/sh
# $Id: ckcf.sh,v 4.10 1996/07/14 18:49:25 kb207252 Exp $
# By ksb and Larry Vaught (ldv@sequent.com) 1992
#
# PS: We know "comm" would be smarter.

BASE=${1-turnin.cf}

grep -v '#' < $BASE | awk -F: '{ print $4 }' |
while read group
do
	grep "^$group:" /etc/group >/dev/null && continue
	echo "$0: $BASE: $group: no such group"
done

grep -v '#' < $BASE | awk -F: '{ print $2 }' |
while read user
do
	grep "^$user:" /etc/passwd >/dev/null && continue
	echo "$0: $BASE: $user: no such user"
done

exit 0
