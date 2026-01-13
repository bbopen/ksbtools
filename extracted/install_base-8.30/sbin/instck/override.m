#!mkcmd
# $Id: override.m,v 8.24 2009/12/10 18:42:13 ksb Exp $
# When we are building the RPM we can't use mkcmd to find these, so we
# for the expression to be a literal path. -- ksb

key "install_path" 8 {
	"/usr/local/bin/install"
}
key "purge_path" 8 {
	"/usr/local/sbin/purge"
#	"/usr/local/etc/purge"
}
