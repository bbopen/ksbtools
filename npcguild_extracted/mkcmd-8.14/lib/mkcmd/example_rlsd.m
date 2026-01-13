#!mkcmd
# $Id: example_rlsd.m,v 8.12 1998/09/25 14:49:11 ksb Exp $
# Example RPC ls server taken from /usr/share/examples/sunrpc/dir
# on a FreeBSD host.  Converter to mkcmd by ksb.
from "<sys/dir.h>"

require "util_errno.m" "std_help.m"
require "example_rls-inc,xdr,server.x"

type "rpc_server" named "dirprog_1" {
	key 1 {
		"DIRPROG" "DIRVERS"
	}
}

%i
extern char *malloc();
extern char *strcpy();
%%

%c
readdir_res *
readdir_1(dirname)
	nametype *dirname;
{
	register DIR *dirp;
	register struct direct *d;
	register namelist nl;
	register namelist *nlp;
	static readdir_res res;	/* we return the address of this */

	/* Open directory, return failure when we can't
	 */
	dirp = opendir(*dirname);
	if (dirp == NULL) {
		res.errno = errno;
		return (&res);
	}

	/* Free previous result
	 */
	xdr_free(xdr_readdir_res, &res);

	/* Collect directory entries
	 */
	nlp = &res.readdir_res_u.list;
	while (d = readdir(dirp)) {
		nl = *nlp = (namenode *) malloc(sizeof(namenode));
		nl->name = malloc(strlen(d->d_name)+1);
		strcpy(nl->name, d->d_name);
		nlp = &nl->next;
	}
	*nlp = NULL;

	/* Return the result
	 */
	res.errno = 0;
	closedir(dirp);
	return (&res);
}
