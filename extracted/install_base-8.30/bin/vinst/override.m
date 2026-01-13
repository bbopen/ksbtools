#!mkcmd
# $Id: override.m,v 1.22 2009/12/10 20:51:25 ksb Exp $
# Allow vinst to compile before the rest of install_base is installed --ksb

key "install_path" 8 {
	"/usr/local/bin/install"
}
key "installus_path" 8 {
	"/usr/local/sbin/installus"
#	"/usr/local/etc/installus"
}
key "instck_path" 8 {
	"/usr/local/sbin/instck"
#	"/usr/local/etc/instck"
}
