# $Id: header.m4,v 1.1 1998/11/19 13:27:34 ksb Exp $
# $Compile: cdmdmctl +rts +dtr /dev/ttyC2? && conserver -C %f ; sleep 90
#
# Change this file in the master source
#	$Source: /usr/msrc/usr/local/lib/conserver.cf/RCS/header.m4,v $
# then push it to info.sa.fedex.com and restart the console server
# there, then push it to the effected host server and restart that one.
# Others only if needed (viz. change of allowed list).
#
# TODO: the Benefits FTP servers are not console served anymore...
#
# List of consoles we serve:
# name :[epasswd]:tty[@host]:baud[flow][parity]:logfile:[group|+][:power:line]
