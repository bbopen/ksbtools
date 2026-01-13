# $Id: kachina.sac.fedex.com,v 2.2 2000/05/02 20:16:21 ksb Exp $
# host kachina (PC HPUX11 the 24 CD ports 2 ttys, Brussles)
# our console is a local bitmapped display and a tty00
#k_00::/dev/tty00@kachina.sac.fedex.com:9600p:/var/consoles/k_00:0
k_01::/dev/tty01@kachina.sac.fedex.com:9600p:/var/consoles/k_01:
# MUX
ec101::/dev/ttyC40h@kachina.sac.fedex.com:9600p:/var/consoles/ec101:+
ec102::/dev/ttyC41h@kachina.sac.fedex.com:9600p:/var/consoles/ec102:
www101::/dev/ttyC42h@kachina.sac.fedex.com:9600p:/var/consoles/www101:
www102::/dev/ttyC43h@kachina.sac.fedex.com:9600p:/var/consoles/www102:
k_44::/dev/ttyC44h@kachina.sac.fedex.com:9600p:/var/consoles/k_44:
k_45::/dev/ttyC45h@kachina.sac.fedex.com:9600p:/var/consoles/k_45:
k_46::/dev/ttyC46h@kachina.sac.fedex.com:9600p:/var/consoles/k_46:
k_47::/dev/ttyC47h@kachina.sac.fedex.com:9600p:/var/consoles/k_47:
k_48::/dev/ttyC48h@kachina.sac.fedex.com:9600p:/var/consoles/k_48:
k_49::/dev/ttyC49h@kachina.sac.fedex.com:9600p:/var/consoles/k_49:
k_4a::/dev/ttyC4ah@kachina.sac.fedex.com:9600p:/var/consoles/k_4a:
k_4b::/dev/ttyC4bh@kachina.sac.fedex.com:9600p:/var/consoles/k_4b:
k_4c::/dev/ttyC4ch@kachina.sac.fedex.com:9600p:/var/consoles/k_4c:
k_4d::/dev/ttyC4dh@kachina.sac.fedex.com:9600p:/var/consoles/k_4d:
k_4e::/dev/ttyC4eh@kachina.sac.fedex.com:9600p:/var/consoles/k_4e:
k_4f::/dev/ttyC4fh@kachina.sac.fedex.com:9600p:/var/consoles/k_4f:
k_4g::/dev/ttyC4gh@kachina.sac.fedex.com:9600p:/var/consoles/k_4g:
k_4h::/dev/ttyC4hh@kachina.sac.fedex.com:9600p:/var/consoles/k_4h:
k_4i::/dev/ttyC4ih@kachina.sac.fedex.com:9600p:/var/consoles/k_4i:
k_4j::/dev/ttyC4jh@kachina.sac.fedex.com:9600p:/var/consoles/k_4j:
k_4k::/dev/ttyC4kh@kachina.sac.fedex.com:9600p:/var/consoles/k_4k:
k_4l::/dev/ttyC4lh@kachina.sac.fedex.com:9600p:/var/consoles/k_4l:
k_4m::/dev/ttyC4mh@kachina.sac.fedex.com:9600p:/var/consoles/k_4m:
k_4n::/dev/ttyC4nh@kachina.sac.fedex.com:9600p:/var/consoles/k_4n:
k_4o::/dev/ttyC4oh@kachina.sac.fedex.com:9600p:/var/consoles/k_4o:
k_4p::/dev/ttyC4ph@kachina.sac.fedex.com:9600p:/var/consoles/k_4p:
k_4q::/dev/ttyC4qh@kachina.sac.fedex.com:9600p:/var/consoles/k_4q:
k_4r::/dev/ttyC4rh@kachina.sac.fedex.com:9600p:/var/consoles/k_4r:
k_4s:ZHrLzmXPNsSR.:/dev/ttyC4sh@kachina.sac.fedex.com:9600p:/var/consoles/k_4s:+
k_4t::/dev/ttyC4th@kachina.sac.fedex.com:9600p:/var/consoles/k_4t:
e1.fw::/dev/ttyC4uh@kachina.sac.fedex.com:9600p:/var/consoles/e1.fw:
e2.fw::/dev/ttyC4vh@kachina.sac.fedex.com:9600p:/var/consoles/e2.fw:
kachina.w::|exec /usr/local/lib/labwatch.boot@kachina.sac.fedex.com:9600p:/usr/adm/labwatch.log:+
