#!mkcmd
# $Id: example_rls.m,v 8.13 2002/04/12 15:03:26 ksb Exp $
# An example interface to an RPC base network protocol -- ksb
# Copied from the FreeBSD /usr/share/example/sunrpc/dir directory

require "std_help.m"
require "example_rls-inc,xdr,client.x"

type "rpc_client" named "cl" "server" {
	key 1 {
		2 "DIRPROG" "DIRVERS"
	}
	help "host to read directories from"
}

# realy should be an option, but we follow the example interface -- ksb
left "cl" {
}

every {
	named "Rls"
	help "directory to ls"
}

%c
static void
Rls(dir)
char *dir;
{
	readdir_res *result;
	auto namelist nl;

	/* Call the remote procedure "readdir" on the server
	 */
	result = readdir_1(&dir, cl);
	if ((readdir_res *)0 == result) {
		/* An error occurred while calling the server.
	 	 * Print error message and die.
		 */
		clnt_perror(cl, server);
		exit(1);
	}

	/* Okay, we successfully called the remote procedure.
	 */
	if (result->ierrno != 0) {
		/* A remote system error occurred.
		 * Print error message and die.
		 */
		errno = result->ierrno;
		perror(dir);
		exit(1);
	}

	/* Successfuly got a directory listing.
	 * Print it out.
	 */
	for (nl = result->readdir_res_u.list; (namelist)0 != nl; nl = nl->next) {
		printf("%s\n", nl->name);
	}
}
