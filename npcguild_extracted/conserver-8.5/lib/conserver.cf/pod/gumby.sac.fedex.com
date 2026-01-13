# $Id: gumby.sac.fedex.com,v 2.3 2000/02/16 17:25:59 ksb Exp $
# host gumby [g] (HP 715 with a ST-1032 32 ports, I2C)
# our console /dev/tty26@trixie.sac.fedex.com 9600p
g_00::/dev/tty00@gumby.sac.fedex.com:9600p:/archive/consoles/g_00:0
g_01::/dev/tty01@gumby.sac.fedex.com:9600p:/archive/consoles/g_01:
powerf::/dev/ttyC20@gumby.sac.fedex.com:9600p:/archive/consoles/powerf:+
cert2::/dev/ttyC21@gumby.sac.fedex.com:9600p:/archive/consoles/cert2:+
ivapptest1::/dev/ttyC22@gumby.sac.fedex.com:9600p:/archive/consoles/ivapptest1:
ivdbtest2::/dev/ttyC23@gumby.sac.fedex.com:9600p:/archive/consoles/ivdbtest2:
g_24::/dev/ttyC24@gumby.sac.fedex.com:9600p:/archive/consoles/g_24:
g_25::/dev/ttyC25@gumby.sac.fedex.com:9600p:/archive/consoles/g_25:
g_26::/dev/ttyC26@gumby.sac.fedex.com:9600p:/archive/consoles/g_26:
g_27::/dev/ttyC27@gumby.sac.fedex.com:9600p:/archive/consoles/g_27:
g_28::/dev/ttyC28@gumby.sac.fedex.com:9600p:/archive/consoles/g_28:
g_29::/dev/ttyC29@gumby.sac.fedex.com:9600p:/archive/consoles/g_29:+
g_2a::/dev/ttyC2a@gumby.sac.fedex.com:9600p:/archive/consoles/g_2a:
g_2b::/dev/ttyC2b@gumby.sac.fedex.com:9600p:/archive/consoles/g_2b:
g_2c::/dev/ttyC2c@gumby.sac.fedex.com:9600p:/archive/consoles/g_2c:
g_2d::/dev/ttyC2d@gumby.sac.fedex.com:9600p:/archive/consoles/g_2d:
g_2e::/dev/ttyC2e@gumby.sac.fedex.com:9600p:/archive/consoles/g_2e:
g_2f::/dev/ttyC2f@gumby.sac.fedex.com:9600p:/archive/consoles/g_2f:
g_2g::/dev/ttyC2g@gumby.sac.fedex.com:9600p:/archive/consoles/g_2g:
g_2h::/dev/ttyC2h@gumby.sac.fedex.com:9600p:/archive/consoles/g_2h:
g_2i::/dev/ttyC2i@gumby.sac.fedex.com:9600p:/archive/consoles/g_2i:
g_2j::/dev/ttyC2j@gumby.sac.fedex.com:9600p:/archive/consoles/g_2j:
g_2k::/dev/ttyC2k@gumby.sac.fedex.com:9600p:/archive/consoles/g_2k:
g_2l::/dev/ttyC2l@gumby.sac.fedex.com:9600p:/archive/consoles/g_2l:
g_2m::/dev/ttyC2m@gumby.sac.fedex.com:9600p:/archive/consoles/g_2m:
g_2n::/dev/ttyC2n@gumby.sac.fedex.com:9600p:/archive/consoles/g_2n:
g_2o::/dev/ttyC2o@gumby.sac.fedex.com:9600p:/archive/consoles/g_2o:
g_2p::/dev/ttyC2p@gumby.sac.fedex.com:9600p:/archive/consoles/g_2p:
g_2q::/dev/ttyC2q@gumby.sac.fedex.com:9600p:/archive/consoles/g_2q:
g_2r::/dev/ttyC2r@gumby.sac.fedex.com:9600p:/archive/consoles/g_2r:
g_2s::/dev/ttyC2s@gumby.sac.fedex.com:9600p:/archive/consoles/g_2s:
g_2t::/dev/ttyC2t@gumby.sac.fedex.com:9600p:/archive/consoles/g_2t:
g_2u::/dev/ttyC2u@gumby.sac.fedex.com:9600p:/archive/consoles/g_2u:
wadm6::/dev/ttyC2v@gumby.sac.fedex.com:9600p:/archive/consoles/wadm6:
gumby.w::|exec /usr/local/lib/labwatch.boot@gumby.sac.fedex.com:9600p:/usr/adm/labwatch:+
