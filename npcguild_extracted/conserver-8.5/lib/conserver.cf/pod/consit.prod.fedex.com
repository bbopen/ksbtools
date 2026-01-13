# $Id: consit.prod.fedex.com,v 2.1 1999/03/16 19:23:48 ksb Exp $
# host consit.prod [a] (HP715, 16 ports + 2 builtin, I2E)
# our console is lost
a_01::/dev/tty01@consit.prod.fedex.com:9600p:/archive/consoles/_01:
gateit01:YGbBOv.ycFcIk:/dev/ttyc50@consit.prod.fedex.com:9600p:/archive/consoles/gateit01.log:+
tchit01::/dev/ttyc51@consit.prod.fedex.com:38400p:/archive/consoles/tchit01.log:
datait01::/dev/ttyc52@consit.prod.fedex.com:9600p:/archive/consoles/datait01.log:
t101::/dev/ttyc53@consit.prod.fedex.com:38400p:/archive/consoles/t101.log:
debug01::/dev/ttyc54@consit.prod.fedex.com:38400p:/archive/consoles/debug01.log:
modem01::/dev/ttyc55@consit.prod.fedex.com:38400p:/archive/consoles/modem01.log:
# TCH 02 + support
gateit02:YGbBOv.ycFcIk:/dev/ttyc58@consit.prod.fedex.com:9600p:/archive/consoles/gateit02.log:+
tchit02::/dev/ttyc59@consit.prod.fedex.com:57600p:/archive/consoles/tchit02.log:
datait02::/dev/ttyc5a@consit.prod.fedex.com:9600p:/archive/consoles/datait02.log:
t102::/dev/ttyc5b@consit.prod.fedex.com:38400p:/archive/consoles/t102.log:
debug02::/dev/ttyc5c@consit.prod.fedex.com:38400p:/archive/consoles/debug02.log:
modem02::/dev/ttyc5d@consit.prod.fedex.com:38400p:/archive/consoles/modem02.log:
# spare port for arc1.dscm
dscmarc1::/dev/ttyc57@consit.prod.fedex.com:9600p:/archive/consoles/dscmac1.log:3:
# fpdata1 support
a_56:YGbBOv.ycFcIk:/dev/ttyc56@consit.prod.fedex.com:9600p:/archive/consoles/_56.log:+
fpdata1::/dev/ttyc5e@consit.prod.fedex.com:9600p:/archive/consoles/fpdata1.log:
fpdata2::/dev/ttyc5f@consit.prod.fedex.com:9600p:/archive/consoles/fpdata2.log:
