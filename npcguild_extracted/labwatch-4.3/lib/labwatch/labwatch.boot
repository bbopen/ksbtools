#!/bin/sh
# $Id: labwatch.boot,v 1.2 1998/07/30 01:42:52 root Exp $
# start the labwatch Ops interface on a console server virtual server port.
#

stty erase '^H' kill '^U' susp undef dsusp undef crt
trap "" 2

CF=labwatch.cf
case `hostname` in
tara*)
	CF=tara.cf ;;
groobee*)
	CF=groobee.cf ;;
*)
	# echo 1>&2 "We do not know what to pick, try $CF" ;;
esac

date +"Labwatch started on %d %b %Y at %H:%M"
/usr/local/etc/labwatch -n -w 300 -C /usr/local/lib/$CF
date +"Labwatch terminated on %d %b %Y at %H:%M"
