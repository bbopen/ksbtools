# $Id: puffball.sac.fedex.com,v 2.11 1999/11/10 19:34:17 root Exp $
# host puffball.sac [u] (HP 715, 1 ST-1032 32 ports, I2C)
# our console is /dev/ttyC28@trixie.sac.fedex.com 9600p
u_01::/dev/tty01@puffball.sac.fedex.com:9600p:/archive/consoles/u_01:
powerh::/dev/ttyC20@puffball.sac.fedex.com:9600p:/archive/consoles/powerh:
a1.fw:ZHrLzmXPNsSR.:/dev/ttyC21@puffball.sac.fedex.com:9600p:/archive/consoles/a1.fw:+
d1.fw::/dev/ttyC22@puffball.sac.fedex.com:9600p:/archive/consoles/d1.fw:
v1.fw::/dev/ttyC2g@puffball.sac.fedex.com:9600p:/archive/consoles/v1.fw:
u_2h::/dev/ttyC2h@puffball.sac.fedex.com:9600p:/archive/consoles/u_2h:
secid3.fipl::/dev/ttyC2e@puffball.sac.fedex.com:9600p:/archive/consoles/secid3:
ldap1.vpn::/dev/ttyC2i@puffball.sac.fedex.com:9600p:/archive/consoles/ldap1.vpn:
www21::/dev/ttyC23@puffball.sac.fedex.com:9600p:/archive/consoles/www21:+
www23::/dev/ttyC24@puffball.sac.fedex.com:9600p:/archive/consoles/www23:
www25::/dev/ttyC25@puffball.sac.fedex.com:9600p:/archive/consoles/www25:
www27::/dev/ttyC26@puffball.sac.fedex.com:9600p:/archive/consoles/www27:
www29::/dev/ttyC27@puffball.sac.fedex.com:9600p:/archive/consoles/www29:
www31::/dev/ttyC28@puffball.sac.fedex.com:9600p:/archive/consoles/www31:
www33::/dev/ttyC29@puffball.sac.fedex.com:9600p:/archive/consoles/www33:
www35::/dev/ttyC2a@puffball.sac.fedex.com:9600p:/archive/consoles/www35:
ship23::/dev/ttyC2b@puffball.sac.fedex.com:9600p:/archive/consoles/ship23:+
ship24::/dev/ttyC2c@puffball.sac.fedex.com:9600p:/archive/consoles/ship24:
ship25::/dev/ttyC2d@puffball.sac.fedex.com:9600p:/archive/consoles/ship25:
mx8::/dev/ttyC2f@puffball.sac.fedex.com:9600p:/archive/consoles/mx8::B1:powerh
u_2j::/dev/ttyC2j@puffball.sac.fedex.com:9600p:/archive/consoles/u_2j:
u_2k::/dev/ttyC2k@puffball.sac.fedex.com:9600p:/archive/consoles/u_2k:
u_2l::/dev/ttyC2l@puffball.sac.fedex.com:9600p:/archive/consoles/u_2l:
u_2m::/dev/ttyC2m@puffball.sac.fedex.com:9600p:/archive/consoles/u_2m:
frcdb2::/dev/ttyC2n@puffball.sac.fedex.com:9600p:/archive/consoles/frcdb2:
comm5::/dev/ttyC2o@puffball.sac.fedex.com:9600p:/archive/consoles/comm5:
comm7::/dev/ttyC2p@puffball.sac.fedex.com:9600p:/archive/consoles/comm7:
app3::/dev/ttyC2q@puffball.sac.fedex.com:9600p:/archive/consoles/app3:
app5::/dev/ttyC2r@puffball.sac.fedex.com:9600p:/archive/consoles/app5:
appstage::/dev/ttyC2s@puffball.sac.fedex.com:9600p:/archive/consoles/appstage:
dewtest1::/dev/ttyC2t@puffball.sac.fedex.com:9600p:/archive/consoles/dewtest1:
cpstest2::/dev/ttyC2u@puffball.sac.fedex.com:9600p:/archive/consoles/cpstest2:
wadm1::/dev/ttyC2v@puffball.sac.fedex.com:9600p:/archive/consoles/wadm1:
puffball.w::|exec /usr/local/lib/labwatch.boot@puffball.sac.fedex.com:9600p:/usr/adm/labwatch.log:+
