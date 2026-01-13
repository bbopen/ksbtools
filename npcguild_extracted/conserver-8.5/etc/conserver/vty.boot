#!/bin/sh
# $Id: vty.boot,v 8.1 1998/06/07 16:24:55 ksb Exp $
exec </dev/$1 >/dev/$1 2>&1
echo  "* * * New console server on $1 * * *"
date +"* %A %e %b %Y at %H:%M:%S %Z"
echo  "*"

/usr/local/etc/conserver -v

echo  "*"
date +"* %A %e %b %Y at %H:%M:%S %Z"
echo  "TCP/IP cool down of 90 seconds..."
sleep 90
echo  "Console server terminated."

exit 0
