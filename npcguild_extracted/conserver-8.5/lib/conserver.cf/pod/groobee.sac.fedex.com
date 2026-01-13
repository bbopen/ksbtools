# $Id: groobee.sac.fedex.com,v 2.11 2000/01/03 14:09:56 ksb Exp $
# host groobee.sac [b] (HP715 with ST-1032 mux 32 ports + 2 builtin, I2C)
# our console /dev/ttyC22@trixie.sac.fedex.com 9600p
b_01::/dev/tty01@groobee.sac.fedex.com:9600p:/archive/consoles/b_01:0
powerb::/dev/ttyC20@groobee.sac.fedex.com:9600p:/archive/consoles/powerb.log:+
y2k::/dev/ttyC21@groobee.sac.fedex.com:9600p:/archive/consoles/y2k.log:+
comm2::/dev/ttyC22@groobee.sac.fedex.com:9600p:/archive/consoles/comm2.log:
www8::/dev/ttyC23@groobee.sac.fedex.com:9600p:/archive/consoles/www8.log:
ldap2::/dev/ttyC24@groobee.sac.fedex.com:9600p:/archive/consoles/ldap2.log:
www2::/dev/ttyC25@groobee.sac.fedex.com:9600p:/archive/consoles/www2.log:
dew2::/dev/ttyC26@groobee.sac.fedex.com:9600p:/archive/consoles/dew2:
app2::/dev/ttyC27@groobee.sac.fedex.com:9600p:/archive/consoles/app2.log:
b_28::/dev/ttyC28@groobee.sac.fedex.com:9600p:/archive/consoles/b_28:
wapi2::/dev/ttyC29@groobee.sac.fedex.com:9600p:/archive/consoles/wapi2.log:
msrc1::/dev/ttyC2a@groobee.sac.fedex.com:9600p:/archive/consoles/msrc1.log:
fxentrust2::/dev/ttyC2b@groobee.sac.fedex.com:9600p:/archive/consoles/fxentrust2.log:
www10::/dev/ttyC2c@groobee.sac.fedex.com:9600p:/archive/consoles/www10.log:
mx1::/dev/ttyC2d@groobee.sac.fedex.com:9600p:/archive/consoles/mx1::F1:powerb
dev1::/dev/ttyC2e@groobee.sac.fedex.com:9600p:/archive/consoles/dev1:
ver1::/dev/ttyC2f@groobee.sac.fedex.com:9600p:/archive/consoles/ver1:
cc1::/dev/ttyC2g@groobee.sac.fedex.com:9600p:/archive/consoles/cc1.log:
crc2::/dev/ttyC2h@groobee.sac.fedex.com:9600p:/archive/consoles/crc2.log:
app7::/dev/ttyC2j@groobee.sac.fedex.com:9600p:/archive/consoles/app7:
b_2k::/dev/ttyC2k@groobee.sac.fedex.com:9600p:/archive/consoles/b_2k:
frcdb1::/dev/ttyC2l@groobee.sac.fedex.com:9600p:/archive/consoles/frcdb1:
b_2m::/dev/ttyC2m@groobee.sac.fedex.com:9600p:/archive/consoles/b_2m:
mx5::/dev/ttyC2n@groobee.sac.fedex.com:9600p:/archive/consoles/mx5::F2:powerb
b_2o::/dev/ttyC2o@groobee.sac.fedex.com:9600p:/archive/consoles/b_2o:
b_2p::/dev/ttyC2p@groobee.sac.fedex.com:9600p:/archive/consoles/b_2p:
ftp1.ec:KBomzlCn5O3r2:/dev/ttyC2i@groobee.sac.fedex.com:9600p:/archive/consoles/ftp1.ec:+
h1.fw:ZHrLzmXPNsSR.:/dev/ttyC2q@groobee.sac.fedex.com:9600p:/archive/consoles/h1.fw.log:+
b_2r::/dev/ttyC2r@groobee.sac.fedex.com:9600p:/archive/consoles/b_2a:
b_2s::/dev/ttyC2s@groobee.sac.fedex.com:9600p:/archive/consoles/b_2s:+
ec5::/dev/ttyC2t@groobee.sac.fedex.com:9600p:/archive/consoles/ec5:
ec3::/dev/ttyC2u@groobee.sac.fedex.com:9600p:/archive/consoles/ec3:
wadm4::/dev/ttyC2v@groobee.sac.fedex.com:9600p:/archive/consoles/wadm4.log:
groobee.w::|exec /usr/local/lib/labwatch.boot@groobee.sac.fedex.com:9600p:/usr/adm/labwatch.log:+
