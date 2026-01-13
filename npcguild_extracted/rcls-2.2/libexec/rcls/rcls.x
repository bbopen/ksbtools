/* $Id: rcls.x,v 2.0 2000/06/01 00:15:04 ksb Exp $
 *
 * Remote Customer Login Services -- provide a remote www customer a	(ksb)
 * simple login service.  LDAP was chewing my face off so I coded
 * a RPC based solutoin I could understand.
 */

#if defined(BUILD_CLASS_HDR)
/* if wee are building the class header we can't see the mkcmd
 * code for the Cargv or Cstring types, so we force a link here.
 */
%typedef char *Cstring;
%typedef char **Cargv;
#endif

struct create_param {
	Cstring pclogin;
	Cstring pcpassword;
	Cargv ppcprotect;
	Cargv ppcpublic;
	Cstring pcclass;
};
struct create_res {
	int iret;
	Cargv ppcprotect;
	Cargv ppcpublic;
};

struct login_param {
	Cstring pclogin;
	Cstring pcpassword;
};
struct login_res {
	int iret;
	Cargv ppcprotect;
	Cargv ppcpublic;
};

struct rename_param {
	Cstring pclogin;
	Cstring pcpassword;
	Cstring pcnewlogin;
	Cstring pcnewpassword;
};
struct rename_res {
	int iret;
};

struct update_param {
	Cstring pclogin;
	Cstring pcpassword;
	Cargv ppcprotect;
	Cargv ppcpublic;
};
/* use create_res again */

struct info_param {
	Cstring pcclass;
	Cstring pcopts;
};
struct info_res {
	int iret;
	Cstring pcdata;
};

program RCLSPROG {
	version RCLSVERS {
		create_res
		loginrcreate(create_param) = 1;

		login_res
		loginropen(login_param) = 2;

		rename_res
		loginrrename(rename_param) = 3;

		create_res
		loginrupdate(update_param) = 4;

		void
		loginrsync(void) = 5;

		info_res
		classrinfo(info_param) = 10;
	} = 1;
} = 307350;
