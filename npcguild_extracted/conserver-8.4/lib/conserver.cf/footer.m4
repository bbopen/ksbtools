`# list of clients we allow
# type: machines/domains/ip
trust: 127.0.0.1
allowed: sa.fedex.com dpd.fedex.com dspd.fedex.com sac.fedex.com infosec.fedex.com
allowed: prod.fedex.com telecom.fedex.com network.fedex.com
allowed: svr6.dscm.fedex.com svr5.dscm.fedex.com
'ifelse(EC,ifelse(
SHORTHOST,`einstein',`EC',
SHORTHOST,`enqa',`EC',
SHORTHOST,`groobee',`EC',
SHORTHOST,`tara',`EC',`nope'),
`allowed: enqa.corp.fedex.com einstein.corp.fedex.com',
``# no extra allows'')`
'dnl
