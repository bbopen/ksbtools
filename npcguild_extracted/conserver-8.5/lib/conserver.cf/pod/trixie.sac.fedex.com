# $Id: trixie.sac.fedex.com,v 2.4 1999/07/30 13:22:48 ksb Exp $
# host trixie [i] (HP 715, a ST-1032 32 ports + 2 builtin, I2C)
# our console is lost
i_00::/dev/tty00@trixie.sac.fedex.com:9600p:/archive/consoles/i_00:0
i_01::/dev/tty01@trixie.sac.fedex.com:9600p:/archive/consoles/i_01:
power::/dev/ttyC20@trixie.sac.fedex.com:9600p:/archive/consoles/power.log:+
zveegee::/dev/ttyC21@trixie.sac.fedex.com:9600p:/archive/consoles/zveegee.log:+
groobee::/dev/ttyC22@trixie.sac.fedex.com:9600p:/archive/consoles/groobee.log:
minga::/dev/ttyC23@trixie.sac.fedex.com:9600p:/archive/consoles/minga.log:
tara::/dev/ttyC24@trixie.sac.fedex.com:9600p:/archive/consoles/tara.log:
goo::/dev/ttyC25@trixie.sac.fedex.com:9600p:/archive/consoles/goo.log:
gumby::/dev/ttyC26@trixie.sac.fedex.com:9600p:/archive/consoles/gumby.log:
nopey::/dev/ttyC27@trixie.sac.fedex.com:9600p:/archive/consoles/nopey.log:
puffball::/dev/ttyC28@trixie.sac.fedex.com:9600p:/archive/consoles/puffball.log:
chalktalk::/dev/ttyC29@trixie.sac.fedex.com:9600p:/archive/consoles/chalktalk:+
denali::/dev/ttyC2a@trixie.sac.fedex.com:9600p:/archive/consoles/denali:
i_2b::/dev/ttyC2b@trixie.sac.fedex.com:9600p:/archive/consoles/i_2b:
i_2c::/dev/ttyC2c@trixie.sac.fedex.com:9600p:/archive/consoles/i_2c:
i_2d::/dev/ttyC2d@trixie.sac.fedex.com:9600p:/archive/consoles/i_2d:
i_2e::/dev/ttyC2e@trixie.sac.fedex.com:9600p:/archive/consoles/i_2e:
i_2f::/dev/ttyC2f@trixie.sac.fedex.com:9600p:/archive/consoles/i_2f:
i_2g::/dev/ttyC2g@trixie.sac.fedex.com:9600p:/archive/consoles/i_2g:
i_2h::/dev/ttyC2h@trixie.sac.fedex.com:9600p:/archive/consoles/i_2h:
i_2i::/dev/ttyC2i@trixie.sac.fedex.com:9600p:/archive/consoles/i_2i:
i_2j::/dev/ttyC2j@trixie.sac.fedex.com:9600p:/archive/consoles/i_2j:
i_2k::/dev/ttyC2k@trixie.sac.fedex.com:9600p:/archive/consoles/i_2k:
i_2l::/dev/ttyC2l@trixie.sac.fedex.com:9600p:/archive/consoles/i_2l:
i_2m::/dev/ttyC2m@trixie.sac.fedex.com:9600p:/archive/consoles/i_2m:
i_2n::/dev/ttyC2n@trixie.sac.fedex.com:9600p:/archive/consoles/i_2n:
i_2o::/dev/ttyC2o@trixie.sac.fedex.com:9600p:/archive/consoles/i_2o:
i_2p::/dev/ttyC2p@trixie.sac.fedex.com:9600p:/archive/consoles/i_2p:
i_2q::/dev/ttyC2q@trixie.sac.fedex.com:9600p:/archive/consoles/i_2q:
i_2r::/dev/ttyC2r@trixie.sac.fedex.com:9600p:/archive/consoles/i_2r:
i_2s::/dev/ttyC2s@trixie.sac.fedex.com:9600p:/archive/consoles/i_2s:
i_2t::/dev/ttyC2t@trixie.sac.fedex.com:9600p:/archive/consoles/i_2t:
i_2u::/dev/ttyC2u@trixie.sac.fedex.com:9600p:/archive/consoles/i_2u:
i_2v::/dev/ttyC2v@trixie.sac.fedex.com:9600p:/archive/consoles/i_2v:
trixie.w::|exec /usr/local/lib/labwatch.boot@trixie.sac.fedex.com:9600p:/usr/adm/labwatch.log:+
