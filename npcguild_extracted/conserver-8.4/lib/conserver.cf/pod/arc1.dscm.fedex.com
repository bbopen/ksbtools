# $Id: arc1.dscm.fedex.com,v 2.1 1999/03/16 19:23:48 ksb Exp $
# host arc1.dscm [x] (IBM C10, virtuals only, I2E)
# our console is on consit
arc1_backup:KWfNYVCsFR/VM:|exec /usr/local/etc/autologin -loperator@dscm-arc1.prod.fedex.com:9600p:/usr/adm/dscmarc1_backup.log:
