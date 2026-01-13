# $Id: w12.sac.fedex.com,v 2.1 1999/03/16 19:23:48 ksb Exp $
# w12.sac [E] (FreeBSD PC, 6 ttyports, various for testing)
e1::/dev/ttyd0@w12.sac.fedex.com:9600p:/.home/w12/consoles/e1.log:0
E_d1::/dev/ttyd1@w12.sac.fedex.com:9600p:/.home/w12/consoles/_d1.log:0
e2::/dev/ttyd2@w12.sac.fedex.com:9600p:/.home/w12/consoles/e2.log:0
E_d3::/dev/ttyd3@w12.sac.fedex.com:9600p:/.home/w12/consoles/_d3.log:0
E_d4::/dev/ttyd4@w12.sac.fedex.com:9600p:/.home/w12/consoles/_d4.log:0
e3::/dev/ttyd5@w12.sac.fedex.com:9600p:/.home/w12/consoles/e3.log:0
