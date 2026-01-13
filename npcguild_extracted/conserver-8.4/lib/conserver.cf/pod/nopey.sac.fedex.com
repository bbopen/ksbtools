# $Id: nopey.sac.fedex.com,v 2.10 2000/02/24 20:41:02 kjbanks Exp $
# host nopey.sac [n] (HP 715 with 1 ST-1032 32 ports, I2C)
# out console is /dev/ttyC27@trixie.sac.fedex.com 9600p
n_01::/dev/tty01@nopey.sac.fedex.com:9600p:/archive/consoles/n_01:
powerg::/dev/ttyC20@nopey.sac.fedex.com:9600p:/archive/consoles/powerg:
a2.fw:ZHrLzmXPNsSR.:/dev/ttyC21@nopey.sac.fedex.com:9600p:/archive/consoles/a2.fw:+
d2.fw::/dev/ttyC22@nopey.sac.fedex.com:9600p:/archive/consoles/d2.fw:
v2.fw::/dev/ttyC2g@nopey.sac.fedex.com:9600p:/archive/consoles/v2.fw:
secid4.fipl::/dev/ttyC2e@nopey.sac.fedex.com:9600p:/archive/consoles/secid4:
ldap2.vpn::/dev/ttyC2m@nopey.sac.fedex.com:9600p:/archive/consoles/ldap2.vpn:
www20::/dev/ttyC23@nopey.sac.fedex.com:9600p:/archive/consoles/www20:+
www22::/dev/ttyC24@nopey.sac.fedex.com:9600p:/archive/consoles/www22:
www24::/dev/ttyC25@nopey.sac.fedex.com:9600p:/archive/consoles/www24:
www26::/dev/ttyC26@nopey.sac.fedex.com:9600p:/archive/consoles/www26:
www28::/dev/ttyC27@nopey.sac.fedex.com:9600p:/archive/consoles/www28:
www30::/dev/ttyC28@nopey.sac.fedex.com:9600p:/archive/consoles/www30:
dewtest2::/dev/ttyC29@nopey.sac.fedex.com:9600p:/archive/consoles/dewtest2:
www34::/dev/ttyC2a@nopey.sac.fedex.com:9600p:/archive/consoles/www34:
ship20::/dev/ttyC2b@nopey.sac.fedex.com:9600p:/archive/consoles/ship20:+
ship21::/dev/ttyC2c@nopey.sac.fedex.com:9600p:/archive/consoles/ship21:
ship22::/dev/ttyC2d@nopey.sac.fedex.com:9600p:/archive/consoles/ship22:
mx6::/dev/ttyC2f@nopey.sac.fedex.com:9600p:/archive/consoles/mx6::B1:powerg
dizzy:ksSsqW34CM3HI:/dev/ttyC2h@nopey.sac.fedex.com:9600p:/archive/consoles/dizzy:+
catbert::/dev/ttyC2i@nopey.sac.fedex.com:9600p:/archive/consoles/catbert:
dilbert::/dev/ttyC2j@nopey.sac.fedex.com:9600p:/archive/consoles/dilbert:
n_2k::/dev/ttyC2k@nopey.sac.fedex.com:9600p:/archive/consoles/n_2k:
www36::/dev/ttyC2l@nopey.sac.fedex.com:9600p:/archive/consoles/www36:
wwwstage::/dev/ttyC2n@nopey.sac.fedex.com:9600p:/archive/consoles/wwwstage:+
wwwbeta::/dev/ttyC2o@nopey.sac.fedex.com:9600p:/archive/consoles/wwwbeta1:
log2::/dev/ttyC2p@nopey.sac.fedex.com:9600p:/archive/consoles/log2:
app4::/dev/ttyC2q@nopey.sac.fedex.com:9600p:/archive/consoles/app4:
app6::/dev/ttyC2r@nopey.sac.fedex.com:9600p:/archive/consoles/app6:
msrctest1::/dev/ttyC2s@nopey.sac.fedex.com:9600p:/archive/consoles/msrctest1:
cps4::/dev/ttyC2t@nopey.sac.fedex.com:9600p:/archive/consoles/cps4:
appbeta1::/dev/ttyC2u@nopey.sac.fedex.com:9600p:/archive/consoles/appbeta1:
wadm2::/dev/ttyC2v@nopey.sac.fedex.com:9600p:/archive/consoles/wadm2:
