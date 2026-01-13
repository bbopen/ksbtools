#!mkcmd
# $Id: getopt_key.m,v 8.40 2000/05/31 13:17:28 ksb Exp $
# The common keys between getopt.m and shared.m				(ksb)

key "getopt_switches" 8 init {
	"u_optInd" "u_optarg" "u_optopt" "0"
}

key "envopt" 8 init {
	"u_envopt" "& argc" "& argv" "%a"
}

key "getarg" 8 init {
	"u_getarg" "argc" "argv"
}

key "getopt" 8 init {
	"u_getopt" "argc" "argv" "%:" "%\\"
}

key "getopt_shared" 8 init {
	"0" "static"
}
