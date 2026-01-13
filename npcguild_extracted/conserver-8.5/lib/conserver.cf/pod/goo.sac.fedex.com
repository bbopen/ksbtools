# $Id: goo.sac.fedex.com,v 2.5 2000/06/26 20:41:13 root Exp $
# host goo [o] (HP 715 with a ST-1032 32 ports + builtin, I2C)
# our console /dev/ttyC25@trixie.sac.fedex.com 9600p
o_00::/dev/tty00@goo.sac.fedex.com:9600p:/archive/consoles/o_00:0
o_01::/dev/tty01@goo.sac.fedex.com:9600p:/archive/consoles/o_01:
powere::/dev/ttyC20@goo.sac.fedex.com:9600p:/archive/consoles/powere:+
cert1::/dev/ttyC21@goo.sac.fedex.com:9600p:/archive/consoles/cert1:+
ediint1::/dev/ttyC22@goo.sac.fedex.com:9600p:/archive/consoles/ediint1:
ediint2::/dev/ttyC23@goo.sac.fedex.com:9600p:/archive/consoles/ediint2:
ivapptest2::/dev/ttyC24@goo.sac.fedex.com:9600p:/archive/consoles/ivapptest2:
ivdbtest1::/dev/ttyC25@goo.sac.fedex.com:9600p:/archive/consoles/ivdbtest1:
o_27::/dev/ttyC27@goo.sac.fedex.com:9600p:/archive/consoles/o_27:
o_28::/dev/ttyC28@goo.sac.fedex.com:9600p:/archive/consoles/o_28:
o_29::/dev/ttyC29@goo.sac.fedex.com:9600p:/archive/consoles/o_29:+
o_2a::/dev/ttyC2a@goo.sac.fedex.com:9600p:/archive/consoles/o_2a:
o_2b::/dev/ttyC2b@goo.sac.fedex.com:9600p:/archive/consoles/o_2b:
o_2c::/dev/ttyC2c@goo.sac.fedex.com:9600p:/archive/consoles/o_2c:
o_2d::/dev/ttyC2d@goo.sac.fedex.com:9600p:/archive/consoles/o_2d:
o_2e::/dev/ttyC2e@goo.sac.fedex.com:9600p:/archive/consoles/o_2e:
o_2f::/dev/ttyC2f@goo.sac.fedex.com:9600p:/archive/consoles/o_2f:
o_2g::/dev/ttyC2g@goo.sac.fedex.com:9600p:/archive/consoles/o_2g:
o_2h::/dev/ttyC2h@goo.sac.fedex.com:9600p:/archive/consoles/o_2h:
o_2i::/dev/ttyC2i@goo.sac.fedex.com:9600p:/archive/consoles/o_2i:
o_2j::/dev/ttyC2j@goo.sac.fedex.com:9600p:/archive/consoles/o_2j:
o_2k::/dev/ttyC2k@goo.sac.fedex.com:9600p:/archive/consoles/o_2k:
o_2l::/dev/ttyC2l@goo.sac.fedex.com:9600p:/archive/consoles/o_2l:
o_2m::/dev/ttyC2m@goo.sac.fedex.com:9600p:/archive/consoles/o_2m:
o_2n::/dev/ttyC2n@goo.sac.fedex.com:9600p:/archive/consoles/o_2n:
o_2o::/dev/ttyC2o@goo.sac.fedex.com:9600p:/archive/consoles/o_2o:
o_2p::/dev/ttyC2p@goo.sac.fedex.com:9600p:/archive/consoles/o_2p:
o_2q::/dev/ttyC2q@goo.sac.fedex.com:9600p:/archive/consoles/o_2q:
o_2r::/dev/ttyC2r@goo.sac.fedex.com:9600p:/archive/consoles/o_2r:
o_2s::/dev/ttyC2s@goo.sac.fedex.com:9600p:/archive/consoles/o_2s:
o_2t::/dev/ttyC2t@goo.sac.fedex.com:9600p:/archive/consoles/o_2t:
o_2u::/dev/ttyC2u@goo.sac.fedex.com:9600p:/archive/consoles/o_2u:
wadm5::/dev/ttyC2v@goo.sac.fedex.com:9600p:/archive/consoles/wadm5:
goo.w::|exec /usr/local/lib/labwatch.boot@goo.sac.fedex.com:9600p:/usr/adm/labwatch:+
adm6.fw:ZHrLzmXPNsSR.:/dev/ttyC26@goo.sac.fedex.com:9600p:/archive/consoles/adm6.fw:
