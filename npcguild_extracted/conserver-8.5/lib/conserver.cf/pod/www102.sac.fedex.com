# $Id: www102.sac.fedex.com,v 1.1 1999/06/22 21:18:27 ksb Exp $
# host www102 [y] (Sun Ultra2, a ST-1032 32 ports + 2 builtin, EMEA BRUSSELS)
# our console is on kachina.sac; we are firewall protected!
y_00::/dev/tty00@www102.sac.fedex.com:9600p:/archive/consoles/y_00:0
y_01::/dev/tty01@www102.sac.fedex.com:9600p:/archive/consoles/y_01:
power::/dev/ttyC20@www102.sac.fedex.com:9600p:/archive/consoles/power.log:+
y_21::/dev/ttyC21@www102.sac.fedex.com:9600p:/archive/consoles/y_29:+
y_22::/dev/ttyC22@www102.sac.fedex.com:9600p:/archive/consoles/y_29:
y_23::/dev/ttyC23@www102.sac.fedex.com:9600p:/archive/consoles/y_29:
y_24::/dev/ttyC24@www102.sac.fedex.com:9600p:/archive/consoles/y_29:
y_25::/dev/ttyC25@www102.sac.fedex.com:9600p:/archive/consoles/y_29:
y_26::/dev/ttyC26@www102.sac.fedex.com:9600p:/archive/consoles/y_29:
y_27::/dev/ttyC27@www102.sac.fedex.com:9600p:/archive/consoles/y_29:
y_28::/dev/ttyC28@www102.sac.fedex.com:9600p:/archive/consoles/y_29:
y_29::/dev/ttyC29@www102.sac.fedex.com:9600p:/archive/consoles/y_29:+
y_2a::/dev/ttyC2a@www102.sac.fedex.com:9600p:/archive/consoles/y_2a:
y_2b::/dev/ttyC2b@www102.sac.fedex.com:9600p:/archive/consoles/y_2b:
y_2c::/dev/ttyC2c@www102.sac.fedex.com:9600p:/archive/consoles/y_2c:
y_2d::/dev/ttyC2d@www102.sac.fedex.com:9600p:/archive/consoles/y_2d:
y_2e::/dev/ttyC2e@www102.sac.fedex.com:9600p:/archive/consoles/y_2e:
y_2f::/dev/ttyC2f@www102.sac.fedex.com:9600p:/archive/consoles/y_2f:
y_2g::/dev/ttyC2g@www102.sac.fedex.com:9600p:/archive/consoles/y_2g:
y_2h::/dev/ttyC2h@www102.sac.fedex.com:9600p:/archive/consoles/y_2h:
y_2i::/dev/ttyC2i@www102.sac.fedex.com:9600p:/archive/consoles/y_2i:
y_2j::/dev/ttyC2j@www102.sac.fedex.com:9600p:/archive/consoles/y_2j:
y_2k::/dev/ttyC2k@www102.sac.fedex.com:9600p:/archive/consoles/y_2k:
y_2l::/dev/ttyC2l@www102.sac.fedex.com:9600p:/archive/consoles/y_2l:
y_2m::/dev/ttyC2m@www102.sac.fedex.com:9600p:/archive/consoles/y_2m:
y_2n::/dev/ttyC2n@www102.sac.fedex.com:9600p:/archive/consoles/y_2n:
y_2o::/dev/ttyC2o@www102.sac.fedex.com:9600p:/archive/consoles/y_2o:
y_2p::/dev/ttyC2p@www102.sac.fedex.com:9600p:/archive/consoles/y_2p:
y_2q::/dev/ttyC2q@www102.sac.fedex.com:9600p:/archive/consoles/y_2q:
y_2r::/dev/ttyC2r@www102.sac.fedex.com:9600p:/archive/consoles/y_2r:
y_2s::/dev/ttyC2s@www102.sac.fedex.com:9600p:/archive/consoles/y_2s:
y_2t::/dev/ttyC2t@www102.sac.fedex.com:9600p:/archive/consoles/y_2t:
y_2u::/dev/ttyC2u@www102.sac.fedex.com:9600p:/archive/consoles/y_2u:
y_2v::/dev/ttyC2v@www102.sac.fedex.com:9600p:/archive/consoles/y_2v:
#www102.w::|exec /usr/local/lib/labwatch.boot@www102.sac.fedex.com:9600p:/usr/adm/labwatch.log:+
