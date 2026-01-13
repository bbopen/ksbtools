# $Id: vulcan.sac.fedex.com,v 1.1 1998/10/11 17:35:14 ksb Exp $
# vulcan is dead
# host vulcan.sac [v] (FreeBSD PC with 6 ports)
#v_d0::/dev/ttyd0@vulcan.sac.fedex.com:9600p:/archive/consoles/_d0.log:0
#v_d1::/dev/ttyd1@vulcan.sac.fedex.com:9600p:/archive/consoles/_d1.log:
#s1-clone:JTLYBLI1NLFMg:/dev/ttyd2@vulcan.sac.fedex.com:9600p:/archive/consoles/s1-clone.log:+
#v_d3::/dev/ttyd3@vulcan.sac.fedex.com:9600p:/archive/consoles/_d3.log:
#s2-clone::/dev/ttyd4@vulcan.sac.fedex.com:9600p:/archive/consoles/s2-clone.log:
#v_d5::/dev/ttyd5@vulcan.sac.fedex.com:9600p:/archive/consoles/_d5.log:
