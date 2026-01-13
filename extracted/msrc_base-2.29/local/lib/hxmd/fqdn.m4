dnl $Id: fqdn.m4,v 1.2 2010/07/08 21:40:16 ksb Exp $
dnl This host is site policy driven you must change it to make sense	(ksb)
dnl in terms of your local network and domain naming rules.
dnl
dnl Map a host to a few fully qualified domain names.  These aliases point
dnl to the IP various peers require to connect to the host, which is
dnl site policy.  As an example some hosts which have both "dmz"
dnl (customer facing, the intenet facing zone) and/or a "zmd" (corporate
dnl nework facing interfaces) need to know which name to pick to make a
dnl service request to their peers.
dnl
dnl This file builds macro attribute for the interface aliases for a host,
dnl when called in the context of that HOST from dmz.cf or the like.
dnl
dnl Our top-level-domain  (e.g. example.com)
dnl Site policy: remove leading 2 components
pushdef(`FQDN_TLD',`pushdef(`t',`substr(HOST,expr(1+index(HOST,.)))')substr(t,expr(1+index(t,.)))`'popdef(`t')')dnl
dnl
dnl The network alias we know about (should be in dmz.cf)
define(`ZMD_QUAL',`zmd')dnl
define(`DMZ_QUAL',`dmz')dnl
define(`ADMIN_QUAL',`sac')dnl
define(`HOT_QUAL',`sac')dnl		not used yet
dnl
dnl
dnl Site policy: when SHORTHOST does not contains a dot the FQDN is
dnl the SHORTHOST+ADMIN_QUAL+TDL, and the DNS servers are in TLD, otherwise
dnl it is SHORTHOST+TLD.
pushdef(`FQDN_NAME',
`ifelse(-1,index(SHORTHOST,.),
	`ifelse(yes,SERVICES(primary-dns),`SHORTHOST.FQDN_TLD',`SHORTHOST.ADMIN_QUAL.FQDN_TLD')',
	`SHORTHOST.FQDN_TLD')')dnl
dnl
dnl Since the control list is in terms of ther admin interface they are
dnl the same string.
pushdef(`FQDN_ADMIN',dnl				View from Amdin net
	`HOST')dnl
dnl
dnl View from the ZMD to a this host
pushdef(`FQDN_ZMD',
`ifelse(yes,SERVICES(primary-dns),dnl			External DNS host
	`SHORTHOST.ZMD_QUAL.FQDN_TLD',
	`HOST')')dnl
dnl
dnl View from a DMZ host to this host (with a firewall in the path)
pushdef(`FQDN_DMZ',
`ifelse(yes,SERVICES(primary-dns),dnl			External DNS host
	`SHORTHOST.ZMD_QUAL.FQDN_TLD',
yes,IS_DMZ(DEFNET),dnl					DMZ hosts
	`SHORTHOST.ZMD_QUAL.FQDN_TLD',
	`HOST')')dnl
dnl
dnl View from corpnet (aka. back office), note that external hosts
dnl are matched by the DMZ being the default route here.
pushdef(`FQDN_CORP',
`ifelse(yes,SERVICES(primary-dns),dnl			External DNS host
	`SHORTHOST.DMZ_QUAL.FQDN_TLD',
yes,IS_DMZ(DEFNET),dnl					DMZ hosts/Ext DNS
	`SHORTHOST.DMZ_QUAL.FQDN_TLD',
	`HOST')')dnl
dnl
dnl View from the Internet
pushdef(`FQDN_HOT',
`ifelse(yes,SERVICES(primary-dns),dnl			External DNS host
	`SHORTHOST.DMZ_QUAL.FQDN_TLD',
yes,IS_DMZ(DEFNET),dnl					DMZ hosts
	`SHORTHOST.DMZ_QUAL.FQDN_TLD',
	`HOST')')dnl
dnl
dnl Withdraw ourself when needed
pushdef(FQDN_pop,
	`popdef(`FQDN_TLD')popdef(`FQDN_ADMIN')popdef(`FQDN_ZMD')popdef(`FQDN_DMZ')popdef(`FQDN_CORP')popdef(`FQDN_HOT')popdef(`FQDN_NAME')popdef(`FQDN_pop')')dnl
