#!/bin/ksh
# $Id: autolocal.ksh,v 2.0 1997/01/02 18:30:45 kb207252 Exp $
#
# Build a console server configuration to manage the local machine with
# an autologin shell on a pseudo-tty.  Makes conserver.cf in the std
# place and installs it (with install, of course).
#
# Params are sent to install, BTW.

# ask conserver where the config file needs to be:
WHERE=`conserver -V | sed -n "s,.*: configuration in \\\`\\\\(.*\\\\)',\\1,p"` || {
	echo 1>&2 "$0: cannot find conserver or its config file name"
	exit 1
}
PATH_AL=`whence autologin` || {
	echo 1>&2 "$0: cannot find the autologin binary"
	exit 2
}

# find out what host we are doing
US=`hostname || uname -n` || {
	echo 1>&2 "$0: cannot find hostname"
	exit 3
}

# found host, look for domain (DNS style)
DOMAIN=`expr $US : '[^.]*\.\(.*\)' \| localhost`
NODE=`expr $US : '\([^.]*\)\..*' \| $US`

# where should we hide the log?
if [ -d /var/log ]
then
	C_LOG=/var/log/$NODE
elif [ -d /usr/spool ]
then
	C_LOG=/usr/spool/$NODE
else
	echo 1>&2 "$0: console log mapped to /dev/null"
	C_LOG=/dev/null
fi

# build it and install it in place
#
cat <<END | install -m 644 $@ - $WHERE
# auto generated console configuration by ${LOGNAME-${USER-`id -n -u`}}
# built on `date` for $NODE
# name : [epasswd] : tty[@host] : baud[parity] : device : group
$NODE::|exec $PATH_AL -C@$US:9600p:$C_LOG:0
%%
# list of clients we allow
# type machines
allowed: 127.0.0.1 $US
allowed: $DOMAIN
END

exit
