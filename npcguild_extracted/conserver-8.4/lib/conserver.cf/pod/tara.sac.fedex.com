# $Id: tara.sac.fedex.com,v 2.12 2000/03/17 20:01:08 ksb Exp $
# host tara.sac [r] (HP715, a ST-1032B mux with 32 ports + 2 builtin, I2C)
# our console is /dev/ttyC24@trixie.sac.fedex.com 9600p
r_01::/dev/tty01@tara.sac.fedex.com:9600p:/archive/consoles/r_01:0
powerd::/dev/ttyC20@tara.sac.fedex.com:9600p:/archive/consoles/powerd.log:+
mx3::/dev/ttyC21@tara.sac.fedex.com:9600p:/archive/consoles/mx3::F1:powerd
www5::/dev/ttyC22@tara.sac.fedex.com:9600p:/archive/consoles/www5.log:
ver2::/dev/ttyC23@tara.sac.fedex.com:9600p:/archive/consoles/ver2.log:
r_24::/dev/ttyC24@tara.sac.fedex.com:9600p:/archive/consoles/r_24:
wwwtest2::/dev/ttyC25@tara.sac.fedex.com:9600p:/archive/consoles/wwwtest2.log:
r_26::/dev/ttyC26@tara.sac.fedex.com:9600p:/archive/consoles/r_26.log:
mx7::/dev/ttyC27@tara.sac.fedex.com:9600p:/archive/consoles/mx7::F2:powerd
ebptest1::/dev/ttyC2a@tara.sac.fedex.com:9600p:/archive/consoles/ebptest1.log:
frcdbtest1::/dev/ttyC2b@tara.sac.fedex.com:9600p:/archive/consoles/frcdbtest1:
crc1::/dev/ttyC2c@tara.sac.fedex.com:9600p:/archive/consoles/crc1.log:
www4::/dev/ttyC2d@tara.sac.fedex.com:9600p:/archive/consoles/www4.log:
cc2::/dev/ttyC2e@tara.sac.fedex.com:9600p:/archive/consoles/cc2.log:
www1::/dev/ttyC2f@tara.sac.fedex.com:9600p:/archive/consoles/www1.log:
app8::/dev/ttyC2g@tara.sac.fedex.com:9600p:/archive/consoles/app8:
mq1::/dev/ttyC2h@tara.sac.fedex.com:9600p:/archive/consoles/mq1:
vipstest1::/dev/ttyC2j@tara.sac.fedex.com:9600p:/archive/consoles/vipstest1:
r_2k::/dev/ttyC2k@tara.sac.fedex.com:9600p:/archive/consoles/r_2k.log:
ftp2.ec:KBomzlCn5O3r2:/dev/ttyC2i@tara.sac.fedex.com:9600p:/archive/consoles/ftp2.ec:+
ec4:ZHrLzmXPNsSR.:/dev/ttyC2l@tara.sac.fedex.com:9600p:/archive/consoles/ec4:+
ec6::/dev/ttyC2m@tara.sac.fedex.com:9600p:/archive/consoles/ec6:
cain::/dev/ttyC28@tara.sac.fedex.com:9600p:/archive/consoles/cain.log:
abel::/dev/ttyC29@tara.sac.fedex.com:9600p:/archive/consoles/abel.log:
h2.fw::/dev/ttyC2n@tara.sac.fedex.com:9600p:/archive/consoles/r_2n.log:
old_adm1.fw::/dev/ttyC2v@tara.sac.fedex.com:9600p:/archive/consoles/adm1.fw.log:
adm2.fw::/dev/ttyC2o@tara.sac.fedex.com:9600p:/archive/consoles/adm2.fw.log:
adm3.fw:ZHrLzmXPNsSR.:/dev/ttyC2q@tara.sac.fedex.com:9600p:/archive/consoles/adm3.fw.log:+
r_2s::/dev/ttyC2s@tara.sac.fedex.com:9600p:/archive/consoles/a2.fw.log:
c1.fw::/dev/ttyC2u@tara.sac.fedex.com:9600p:/archive/consoles/c1.fw.log:
c2.fw::/dev/ttyC2t@tara.sac.fedex.com:9600p:/archive/consoles/c2.fw.log:
g1.fw::/dev/ttyC2r@tara.sac.fedex.com:9600p:/archive/consoles/g1.fw.log:
g2.fw::/dev/ttyC2p@tara.sac.fedex.com:9600p:/archive/consoles/g2.fw.log:
tara.w::|exec /usr/local/lib/labwatch.boot@tara.sac.fedex.com:9600p:/usr/adm/labwatch.log:+
