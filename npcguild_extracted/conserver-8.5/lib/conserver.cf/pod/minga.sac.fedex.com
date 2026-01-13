# $Id: minga.sac.fedex.com,v 2.17 2000/02/11 20:10:11 root Exp $
# host minga.sac [m] (HPUX with 1 ST-1032 32 port + 1 builtin ports, I2C)
# our console is /dev/ttyC23@trixie.sac.fedex.com 9600p
m_01::/dev/tty01@minga.sac.fedex.com:9600p:/archive/consoles/_01:0
powerc::/dev/ttyC20@minga.sac.fedex.com:9600p:/archive/consoles/powerc.log:+
m_21::/dev/ttyC21@minga.sac.fedex.com:9600p:/archive/consoles/m_21:+
crc3::/dev/ttyC22@minga.sac.fedex.com:9600p:/archive/consoles/crc3:
wwwalpha::/dev/ttyC23@minga.sac.fedex.com:9600p:/archive/consoles/wwwalpha:
mqtest1::/dev/ttyC24@minga.sac.fedex.com:9600p:/archive/consoles/mqtest1:
www3::/dev/ttyC25@minga.sac.fedex.com:9600p:/archive/consoles/www3.log:
ivdb2::/dev/ttyC26@minga.sac.fedex.com:9600p:/archive/consoles/ivdb2:
mx4::/dev/ttyC27@minga.sac.fedex.com:9600p:/archive/consoles/mx4:+:G1:powerc
apptest2::/dev/ttyC28@minga.sac.fedex.com:9600p:/archive/consoles/apptest2.log:
ebp2::/dev/ttyC29@minga.sac.fedex.com:9600p:/archive/consoles/ebp2.log:
apptest1::/dev/ttyC2a@minga.sac.fedex.com:9600p:/archive/consoles/apptest1.log:
hsttest1::/dev/ttyC2b@minga.sac.fedex.com:9600p:/archive/consoles/hsttest1:
wwwtest1::/dev/ttyC2c@minga.sac.fedex.com:9600p:/archive/consoles/wwwtest1.log:
www6::/dev/ttyC2d@minga.sac.fedex.com:9600p:/archive/consoles/www6.log:
mq2::/dev/ttyC2e@minga.sac.fedex.com:9600p:/archive/consoles/mq2:+
ftp2::/dev/ttyC2f@minga.sac.fedex.com:9600p:/archive/consoles/ftp2.log:
ftp4::/dev/ttyC2g@minga.sac.fedex.com:9600p:/archive/consoles/ftp4.log:
m_2h::/dev/ttyC2h@minga.sac.fedex.com:9600p:/archive/consoles/m_2h.log:
web1::/dev/ttyC2i@minga.sac.fedex.com:9600p:/archive/consoles/web1:
ec2::/dev/ttyC2j@minga.sac.fedex.com:9600p:/archive/consoles/ec2:
ectest1::/dev/ttyC2k@minga.sac.fedex.com:9600p:/archive/consoles/ectest1:
m_2l::/dev/ttyC2l@minga.sac.fedex.com:9600p:/archive/consoles/m_2l.log:
vips2::/dev/ttyC2m@minga.sac.fedex.com:9600p:/archive/consoles/vips2:
vips1::/dev/ttyC2n@minga.sac.fedex.com:9600p:/archive/consoles/vips1:
m_2o::/dev/ttyC2o@minga.sac.fedex.com:9600p:/archive/consoles/m_2o.log:
m_2p::/dev/ttyC2p@minga.sac.fedex.com:9600p:/archive/consoles/m_2p.log:
m_2q::/dev/ttyC2q@minga.sac.fedex.com:9600p:/archive/consoles/m_2q.log:
m_2r::/dev/ttyC2r@minga.sac.fedex.com:9600p:/archive/consoles/m_2r.log:
m_2s::/dev/ttyC2s@minga.sac.fedex.com:9600p:/archive/consoles/m_2s.log:
m_2t::/dev/ttyC2t@minga.sac.fedex.com:9600p:/archive/consoles/m_2t.log:
land:iOwvctJRVHfE6:/dev/ttyC2u@minga.sac.fedex.com:9600p:/archive/consoles/land.log:+
m_2v::/dev/ttyC2v@minga.sac.fedex.com:9600p:/archive/consoles/m_2v.log:
