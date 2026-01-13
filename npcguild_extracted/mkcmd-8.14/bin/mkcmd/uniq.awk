# $Id: uniq.awk,v 8.0 1997/01/29 02:41:57 ksb Exp $
/UNIQUE/{
	if ($3 == "UNIQUE") {
		printf "%s %-24s %3d\n", $1, $2, token;
		token += 1;
	} else {
		print $0;
	}
	next;
}
/^/{
	print $0
}
