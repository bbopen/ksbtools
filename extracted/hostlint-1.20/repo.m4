dnl $Id: repo.m4,v 1.3 2008/03/21 22:40:57 jad Exp $
dnl Select a local host from which we should fetch hostlint-policy	(ksb)
dnl we'll leave and pre-defined one as-is.
ifdef(`HOSTLINT_REPO',`',
`define(`HOSTLINT_REPO',ifelse(yes,SERVICES(radiation),`allen.sac.fedex.com',
yes,SERVICES(emulsion),ftp.zmd.fedex.com,
-1,index(SHORTHOST,test),policy.sac.fedex.com,policytest.sac.fedex.com))')dnl
