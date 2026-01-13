# $Id: s03.sa.fedex.com,v 2.1 1999/03/16 19:23:48 ksb Exp $
# s03.sa [s] (HP715, virtuals and a MUX for the gold net, E365)
# our console on depot.dspd
s_01::/dev/tty01@s03.sa.fedex.com:9600p:/archive/consoles/_01:
s03.w::|exec /usr/local/lib/labwatch.boot@s03.sa.fedex.com:9600p:/usr/adm/labwatch.log:+
qmgr::|exec /usr/local/lib/qmgr.boot@s03.sa.fedex.com:9600p:/usr/adm/job.log:
