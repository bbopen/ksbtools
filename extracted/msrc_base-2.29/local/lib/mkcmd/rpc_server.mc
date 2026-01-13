/* mkcmd interface to install RPC services				(ksb)
 * $Id: rpc_server.mc,v 8.15 2004/03/22 14:45:24 ksb Exp $
 */
int
%%K<rpc_register>1v(u_iSockType)
int u_iSockType;
{
	register SVCXPRT *u_pSX;

%@foreach "rpc_server_list"
%	(void)pmap_unset(%ZK1v, %ZK2v)%;
%@endfor

%@foreach "rpc_server_list"
%	/* transports we need are %ZKss/ql */
%@eval if%ZKss/q/i/ || "udp" == /1.4-$<cat>
	u_pSX = svcudp_create(u_iSockType);
	if ((SVCXPRT *)0 == u_pSX) {
%		fprintf(stderr, "%%s: %qn: cannot create udp service.\n", %b)%;
		exit(1);
	}
%	if (!svc_register(u_pSX, %ZK1v, %ZK2v, %n, IPPROTO_UDP)) %{
%		fprintf(stderr, "%%s: %qn: unable to register (%ZK1v, %ZK2v, udp): %%s\n", %b, %E)%;
		exit(1);
	}
%@endif
%@eval if%ZKss/q/i/ || "tcp" == /1.4-$<cat>
	u_pSX = svctcp_create(u_iSockType, 0, 0);
	if ((SVCXPRT *)0 == u_pSX) {
%		fprintf(stderr, "%%s: %qn: cannot create tcp service.\n", %b)%;
		exit(1);
	}
%	if (!svc_register(u_pSX, %ZK1v, %ZK2v, %n, IPPROTO_TCP)) %{
%		fprintf(stderr, "%%s: %qn: unable to register (%ZK1v, %ZK2v, tcp): %%s\n", %b, %E)%;
		exit(1);
	}
%@endif
%@endfor
	return 0;
}
