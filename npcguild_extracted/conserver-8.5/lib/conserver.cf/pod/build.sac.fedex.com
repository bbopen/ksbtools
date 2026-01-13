# $Id: build.sac.fedex.com,v 2.2 1999/07/30 13:21:28 ksb Exp $
# build.sac [B] (HP715 virtuals and a MUX for the gold net, E365)
# our console on depot.dspd
build.w::|exec /usr/local/lib/labwatch.boot@build.sac.fedex.com:9600p:/usr/adm/labwatch.log:
