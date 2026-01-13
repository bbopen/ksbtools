# $Id: w05.dspd.fedex.com,v 2.1 1999/03/16 19:23:48 ksb Exp $
# w05.dspd [w] (FreebsdPC, 2 ports, E390 )
w_d0::/dev/ttyd0@w05.dspd.fedex.com:9600p:/tmp/_d0.log:0
w_d1::/dev/ttyd1@w05.dspd.fedex.com:9600p:/tmp/_d1.log:
