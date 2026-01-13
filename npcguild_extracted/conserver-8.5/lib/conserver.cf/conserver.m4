# $Id: conserver.m4,v 8.6 2000/06/21 17:24:24 ksb Exp $
include(`header.m4')dnl
ifelse(EC,ifelse(SHORTHOST,`einstein',`EC', SHORTHOST,`enqa',`EC',`not'),
`include(`pod/einstein.corp.fedex.com')dnl	place holder
include(`pod/groobee.sac.fedex.com')dnl		Pod B
include(`pod/tara.sac.fedex.com')dnl		Pod D
',`
include(`pod/s01.sa.fedex.com')dnl		old dspd
include(`pod/s03.sa.fedex.com')dnl		old dspd lab
include(`pod/trixie.sac.fedex.com')dnl		I2C master
include(`pod/zveegee.sac.fedex.com')dnl		Pod A
include(`pod/groobee.sac.fedex.com')dnl		Pod B
include(`pod/minga.sac.fedex.com')dnl		Pod C
include(`pod/tara.sac.fedex.com')dnl		Pod D
include(`pod/goo.sac.fedex.com')dnl		Pod E
include(`pod/gumby.sac.fedex.com')dnl		Pod F
include(`pod/nopey.sac.fedex.com')dnl		Pod G
include(`pod/puffball.sac.fedex.com')dnl	Pod H
include(`pod/chalktalk.sac.fedex.com')dnl	Pod S
include(`pod/denali.sac.fedex.com')dnl		Pod T
include(`pod/kachina.sac.fedex.com')dnl		BRU for EMEA
include(`pod/fortress.dpd.fedex.com')dnl	DPDS
include(`pod/depot.dspd.fedex.com')dnl		SAC Production Lab
include(`pod/w01.sac.fedex.com')dnl		ksb tests here
')dnl
`%%'
include(`footer.m4')dnl
