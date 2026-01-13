# $Id: pokey.dpd.fedex.com,v 2.1 1999/03/16 19:23:48 ksb Exp $
# host pokey [p] (PC FreeBSD, 5 ports, I2W)
# our console is a local VGA display
# ns is grant BTW
ns:6LKqGrXPmYynk:/dev/ttyd0@pokey.dpd.fedex.com:9600p:/archive/consoles/ns.log:0
interlock:89SfZo9O1xJys:/dev/ttyd1@pokey.dpd.fedex.com:9600p:/archive/consoles/interlock.log:+
ilbackup::/dev/ttyd2@pokey.dpd.fedex.com:9600p:/archive/consoles/ilbackup.log:
#p_d3::/dev/ttyd3@pokey.dpd.fedex.com:9600p:/archive/consoles/p_d3.log:
