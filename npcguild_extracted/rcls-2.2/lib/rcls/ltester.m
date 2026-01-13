#!mkcmd
# $Id: ltester.m,v 2.1 2000/06/09 20:08:23 ksb Exp $
# test the rcls rpc daemon --ksb
from "<string.h>"
from "<unistd.h>"
from "<stdlib.h>"
from "<sys/types.h>"
from "<limits.h>"
from "<fcntl.h>"
from "<sys/stat.h>"
from "<db.h>"
from "<rpc/rpc.h>"

from '"rcls_callback.h"'

require "std_help.m" "std_version.m"
require "xdr_Cstring.m" "xdr_Cargv.m"
require "rcls-xdr,header,client.x"

%i
static char rcsid[] =
	"$Id: ltester.m,v 2.1 2000/06/09 20:08:23 ksb Exp $";
%%

char* 'l' {
	named "pcLogin"
	param "login"
	init '"ksb"'
	help "login to act on"
}
char* 'p' {
	named "pcPass"
	param "pass"
	init '"untamo"'
	help "password to use"
}
char* 'o' {
	param "opts"
	named "pcOpts"
	help "info command or options"
}
char* 'c' {
	param "class"
	init '"sso"'
	named "pcClass"
	help "login class parameter"
}
boolean 't' {
	named "fTerse"
	help "truncate to terse attributes for speed tests"
}
boolean 'C' {
	named "fCreate"
	init "0"
	help "create the test account"
}
boolean 'L' {
	named "fLogin"
	help "login the Customer (login, pass)"
}
integer 'Z' {
	named "iLoginReps"
	init "0"
	param "count"
	help "repeat a login count times"
}
boolean 'U' {
	named "fUpdate"
	help "update attributes"
}
boolean 'Q' {
	named "fQuitThat"
	init "0"
	help "shutdown the database (not impl ZZZ)"
}
boolean 'P' {
	named "fChange"
	init "0"
	help "test password change code"
}
boolean 'I' {
	named "fInfo"
	help "information request to class (rr)"
}

boolean 'S' {
	named "fSync"
	help "sync the db at the end"
}
char* 'a' {
	named "pcFormat"
	init '"%04x"'
	param "format"
	help "sprintf format to make login names from interger"
}
integer 'A' {
	named "iAdd"
	track "fAdd"
	param "count"
	help "load tester to build logins"
}
every {
	update "rclsprog_1(%a);"
	param "host"
	help "poke at this server"
}

%c
/* output the returned attribute list					(ksb)
 */
void
AttrDump(char *pcPrefix, char **ppcList)
{
	if ((char **)0 == ppcList) {
		printf("<is NULL>\n");
		return;
	}
	if ((char *)0 == pcPrefix) {
		pcPrefix = "\t";
	}
	while ((char *)0 != *ppcList) {
		printf("%s%s\n", pcPrefix, *ppcList++);
	}
}

/* give the remote customer login service some calls for		(ksb)
 * a debug session
 */
void
rclsprog_1(host)
char *host;
{
	register CLIENT *clnt;
	static char *apcPro[] = {
		"mmn=other",
		(char *)0
	};
	static char *apcPub[] = {
		"sec_q=How many X does it take to Y?",
		"sec_a=1",
		(char *)0
	};
	static char *apcAddPub[] = {
		"pub1=blue",
		"sec_a=none",
		(char *)0
	};
	static char *apcAddPro[] = {
		"pro1=green",
		(char *)0
	};

	if (fTerse) {
		apcPro[0] = (char *)0;
		apcPub[0] = (char *)0;
	}

	clnt = clnt_create(host, RCLSPROG, RCLSVERS, "tcp");
	if ((CLIENT *)0 == clnt) {
		clnt_pcreateerror(host);
		exit(1);
	}

	if (fAdd) {
		register int i;
		register create_res  *result_1;
		auto create_param  loginrcreate_1_arg;
		auto char acName[100];

		loginrcreate_1_arg.pclogin = acName;
		loginrcreate_1_arg.pcpassword = pcPass;
		loginrcreate_1_arg.pcclass = pcClass;
		loginrcreate_1_arg.ppcpublic = apcPub;
		loginrcreate_1_arg.ppcprotect = apcPro;
		for (i = 0; i < iAdd; ++i) {
			sprintf(acName, pcFormat, i);
			result_1 = loginrcreate_1(&loginrcreate_1_arg, clnt);
			if ((create_res *)0 == result_1) {
				clnt_perror(clnt, "create call failed");
				exit(2);
			}
			if (0 != result_1->iret) {
				printf("%d failed with %d\n", i, result_1->iret);
				break;
			}
			printf("%s\n", acName);
		}
	}
	if (fCreate) {
		register create_res  *result_1;
		auto create_param  loginrcreate_1_arg;

		loginrcreate_1_arg.pclogin = pcLogin;
		loginrcreate_1_arg.pcpassword = pcPass;
		loginrcreate_1_arg.pcclass = pcClass;
		loginrcreate_1_arg.ppcpublic = apcPub;
		loginrcreate_1_arg.ppcprotect = apcPro;
		result_1 = loginrcreate_1(&loginrcreate_1_arg, clnt);
		if ((create_res *)0 == result_1) {
			clnt_perror(clnt, "create call failed");
		} else {
			printf("create: %d\nprotected attrs:\n", result_1->iret);
			AttrDump((char *)0, result_1->ppcprotect);
			printf("public attrs:\n");
			AttrDump((char *)0, result_1->ppcpublic);
		}
	}
	if (0 != iLoginReps) {
		register int i;
		register login_res  *result_2;
		auto login_param  loginropen_1_arg;

		loginropen_1_arg.pclogin = pcLogin;
		loginropen_1_arg.pcpassword = pcPass;
		for (i = 0; i < iLoginReps; ++i) {
			result_2 = loginropen_1(&loginropen_1_arg, clnt);
			if ((login_res *)0 == result_2) {
				clnt_perror(clnt, "login call failed");
				break;
			}
		}
	}
	if (fLogin) {
		register login_res  *result_2;
		auto login_param  loginropen_1_arg;

		loginropen_1_arg.pclogin = pcLogin;
		loginropen_1_arg.pcpassword = "bad pass";
		result_2 = loginropen_1(&loginropen_1_arg, clnt);
		if ((login_res *)0 == result_2) {
			clnt_perror(clnt, "login call failed");
		} else {
			printf("login ksb with wrong password returned %d\n", result_2->iret);
			printf("public attrs:\n");
			AttrDump("\t", result_2->ppcpublic);
			printf("protected attrs:\n");
			AttrDump("\t", result_2->ppcprotect);
		}

		loginropen_1_arg.pclogin = pcLogin;
		loginropen_1_arg.pcpassword = pcPass;
		result_2 = loginropen_1(&loginropen_1_arg, clnt);
		if ((login_res *)0 == result_2) {
			clnt_perror(clnt, "login call failed");
		} else {
			printf("login ksb with correct password returned %d\n", result_2->iret);
			printf("public attrs:\n");
			AttrDump("\t", result_2->ppcpublic);
			printf("protected attrs:\n");
			AttrDump("\t", result_2->ppcprotect);
		}
	}

	if (fUpdate) {
		register create_res  *result_4;
		auto update_param  loginrupdate_1_arg;

		loginrupdate_1_arg.pclogin = pcLogin;
		loginrupdate_1_arg.pcpassword = pcPass;
		loginrupdate_1_arg.ppcpublic = apcAddPub;
		loginrupdate_1_arg.ppcprotect = apcAddPro;
		result_4 = loginrupdate_1(&loginrupdate_1_arg, clnt);
		if ((create_res *)0 == result_4) {
			clnt_perror(clnt, "unpdate call failed");
		} else {
			printf("update ksb with correct password returned %d\n", result_4->iret);
			printf("public attrs:\n");
			AttrDump("\t", result_4->ppcpublic);
			printf("protected attrs:\n");
			AttrDump("\t", result_4->ppcprotect);
		}
	}
	if (fChange) {
		register rename_res  *result_3;
		auto rename_param  loginrrename_1_arg;

		loginrrename_1_arg.pclogin = pcLogin;
		loginrrename_1_arg.pcpassword = pcPass;
		loginrrename_1_arg.pcnewlogin = (char *)0;
		loginrrename_1_arg.pcnewpassword = "Untamo";
		result_3 = loginrrename_1(&loginrrename_1_arg, clnt);
		if ((rename_res *)0 == result_3) {
			clnt_perror(clnt, "pass1 call failed");
		} else {
			printf("password change returned %d\n", result_3->iret);
			loginrrename_1_arg.pcpassword = (char *)0;
			loginrrename_1_arg.pcnewpassword = pcPass;
			result_3 = loginrrename_1(&loginrrename_1_arg, clnt);
			if ((rename_res *)0 == result_3) {
				clnt_perror(clnt, "pass2 call failed");
			} else {
				printf("force pass change returned %d\n", result_3->iret);
			}
		}
	}

	if (fInfo) {
		register info_res *info_out;
		auto info_param info_in;

		info_in.pcclass = pcClass;
		info_in.pcopts = pcOpts;
		info_out = classrinfo_1((void *)&info_in, clnt);
		if ((info_res *)0 == info_out) {
			clnt_perror(clnt, "info call failed");
		} else {
			printf("info sso: %s (%d)\n", info_out->pcdata ? info_out->pcdata : "<NULL>", info_out->iret);
		}
	}
	if (fSync) {
		register void  *result_5;
		auto char *loginrsync_1_arg;

		result_5 = loginrsync_1((void *)&loginrsync_1_arg, clnt);
		if ((void *)0 == result_5 ) {
			clnt_perror(clnt, "sync call failed");
		} else {
			printf("called login remote sync\n");
		}
	}
	if (fQuitThat) {
		printf("will quit for stew\n");
	}
	clnt_destroy(clnt);
}
