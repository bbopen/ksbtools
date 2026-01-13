# $Id: zveegee.sac.fedex.com,v 2.14 2000/02/11 20:09:09 root Exp $
# host zveegee.sac [z] (HP715 with ST-1032 mux 32 ports + 2 builtin, E2C)
# our console /dev/ttyC21@trixie.sac.fedex.com 9600p
z_01::/dev/tty01@zveegee.sac.fedex.com:9600p:/archive/consoles/z_01:0
powera::/dev/ttyC20@zveegee.sac.fedex.com:9600p:/archive/consoles/powera.log:+
mx2::/dev/ttyC21@zveegee.sac.fedex.com:9600p:/archive/consoles/mx2:+:G1:powera
z_22::/dev/ttyC22@zveegee.sac.fedex.com:9600p:/archive/consoles/z_22:
ivdb1::/dev/ttyC23@zveegee.sac.fedex.com:9600p:/archive/consoles/ivdb1:
app1::/dev/ttyC24@zveegee.sac.fedex.com:9600p:/archive/consoles/app1.log:
dew1::/dev/ttyC25@zveegee.sac.fedex.com:9600p:/archive/consoles/dew1:
comm1::/dev/ttyC26@zveegee.sac.fedex.com:9600p:/archive/consoles/comm1.log:
ldap1::/dev/ttyC27@zveegee.sac.fedex.com:9600p:/archive/consoles/ldap1.log:
wapi1::/dev/ttyC28@zveegee.sac.fedex.com:9600p:/archive/consoles/wapi1.log:
z_29::/dev/ttyC29@zveegee.sac.fedex.com:9600p:/archive/consoles/z_29:
log1::/dev/ttyC2a@zveegee.sac.fedex.com:9600p:/archive/consoles/log1.log:
wapi3::/dev/ttyC2b@zveegee.sac.fedex.com:9600p:/archive/consoles/wapi3.log:
www9::/dev/ttyC2c@zveegee.sac.fedex.com:9600p:/archive/consoles/www9.log:
fxentrust1::/dev/ttyC2d@zveegee.sac.fedex.com:9600p:/archive/consoles/fxentrust1.log:
ftp3::/dev/ttyC2e@zveegee.sac.fedex.com:9600p:/archive/consoles/ftp3.log:
grace:iOwvctJRVHfE6:/dev/ttyC2f@zveegee.sac.fedex.com:9600p:/archive/consoles/grace.log:+
ftp1::/dev/ttyC2g@zveegee.sac.fedex.com:9600p:/archive/consoles/ftp1.log:
papptest1::/dev/ttyC2h@zveegee.sac.fedex.com:9600p:/archive/consoles/papptest1:
ebp1::/dev/ttyC2i@zveegee.sac.fedex.com:9600p:/archive/consoles/ebp1.log:
olrb1::/dev/ttyC2j@zveegee.sac.fedex.com:9600p:/archive/consoles/olrb1.log:
cctest1::/dev/ttyC2k@zveegee.sac.fedex.com:9600p:/archive/consoles/cc3.log:
ediinttest1::/dev/ttyC2l@zveegee.sac.fedex.com:9600p:/archive/consoles/ediinttest1:
www7::/dev/ttyC2m@zveegee.sac.fedex.com:9600p:/archive/consoles/www7.log:
ec1::/dev/ttyC2n@zveegee.sac.fedex.com:9600p:/archive/consoles/ec1:
wadm0::/dev/ttyC2o@zveegee.sac.fedex.com:9600p:/archive/consoles/wadm0.log:
webtest1::/dev/ttyC2p@zveegee.sac.fedex.com:9600p:/archive/consoles/webtest1:
z_2q::/dev/ttyC2q@zveegee.sac.fedex.com:9600p:/archive/consoles/z_2q:
z_2r::/dev/ttyC2r@zveegee.sac.fedex.com:9600p:/archive/consoles/z_2r:
z_2s::/dev/ttyC2s@zveegee.sac.fedex.com:9600p:/archive/consoles/z_2s:
z_2t::/dev/ttyC2t@zveegee.sac.fedex.com:9600p:/archive/consoles/z_2t:
z_2u::/dev/ttyC2u@zveegee.sac.fedex.com:9600p:/archive/consoles/z_2u:
wadm3::/dev/ttyC2v@zveegee.sac.fedex.com:9600p:/archive/consoles/wadm3.log:
