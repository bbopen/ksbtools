# which ENAD 302 or 13?, ResHalls, or Staff machines (for back-compat)
# $Id: oldopts.m,v 4.0 1994/05/27 13:28:17 ksb Exp $
action '1' {
	exclude 'WmaT'
	update '%r+N = "enad135+138";%r+T'
	help "list first floor of ENAD (historical)"
}

action '3' {
	exclude 'WmaT'
	update '%r+N = "enad302";%r+T'
	help "list third floor of ENAD (historical)"
}

action 'R' {
	exclude 'WmaT'
	update '%r+N = "reshall";%r+T'
	help "list residence hall map (historical)"
}

action 'S' {
	exclude 'WmaT'
	update '%r+N = "staff";%r+T'
	help "list staff IPCs (historical)"
}

action 'A' {
	exclude 'WmaT'
	update '%r+N = "enad302n,enad302s,enad135+138";%r+T'
	help "list default host map"
}
