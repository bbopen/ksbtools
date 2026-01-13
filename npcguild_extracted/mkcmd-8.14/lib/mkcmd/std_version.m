# $Id: std_version.m,v 8.8 1999/06/12 00:43:58 ksb Exp $
# add code to show the user the version under -V			(ksb)
# put
# %%
# static char *rcsid = "...";
# %%
# in your mkcmd file to support this feature.
#
# One may change the arguements to printf with the API key "version_args"
# and "version_format". Or just add a banner with the unnamed key for
# action 'V': just add strings the the key list.
#
action 'V' {
	key "version_format" 2 initialize {
		"%%s:" "%%s"
	}

	key "version_args" 2 initialize {
		"%K<version_format>$+'%+'/e/i' '1.2-$d"
		"%b"
		"rcsid"
	}

	update "printf(%K<version_args>/e1ql);"
	aborts 'exit(0);'
	help "show version information"
}
