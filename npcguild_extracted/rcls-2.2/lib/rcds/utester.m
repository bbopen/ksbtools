#!mkcmd
# $Id: utester.m,v 2.1 2000/06/05 23:49:16 ksb Exp $
# test the rcls rpc daemon --ksb
from "<string.h>"
from "<unistd.h>"
from "<stdlib.h>"
from "<sys/types.h>"
from "<limits.h>"
from "<fcntl.h>"
from "<sys/stat.h>"
from "<rpc/rpc.h>"

from '"rcds_callback.h"'

require "std_help.m" "std_version.m"
require "xdr_Cstring.m" "xdr_Cargv.m"
require "rcds-xdr,header,client.x"

%i
static char rcsid[] =
	"$Id: utester.m,v 2.1 2000/06/05 23:49:16 ksb Exp $";
static char acNull[] = "<NULL>";
%%

boolean 'N' {
	named "fNew"
	help "make the uuid"
}
boolean 'E' {
	named "fExists"
	help "check for existance of the uuid"
}
boolean 'I' {
	named "fIndex"
	help "run an index of the uuid's units"
}
boolean 'M' {
	named "fNewUnit"
	help "new the given unit"
}
string[11] 'U' {
	named "acUuid"
	param "uuid"
	init '"ABC123wxyz"'
	help "ten character UUID to poke at (default %qi)"
}
char* 'u' {
	named "pcUnit"
	param "unit"
	help "unit to poke at"
	init '"unit"'
}
char* 'o' {
	named "pcOpts"
	param "opts"
	help "the options for the call"
}
boolean 'q' {
	named "fUSearch"
	help "search the unit/module tables"
}
boolean 'Q' {
	named "fInfo"
	help "info request to the unit"
}
boolean 'Y' {
	named "fSync"
	help "decache or sync the unit"
}

%i
static char *pcName, *pcValue;

/* parse a NVP into the name and value parts				(ksb)
 */
void
SetNVP(char *pcSet)
{
	pcName = pcSet;
	if ((char *)0 != (pcValue = strchr(pcSet, '='))) {
		*pcValue++ = '\000';
	}
}
%%
function 'S' {
	update "SetNVP(%a);"
	track "fGaveNVP"
	param "name=value"
	help "set the name-value-pair for a put or get, value optional"
}
boolean 'P' {
	named "fPut"
	help "put the NVP set with -S"
}
boolean 'G' {
	named "fGet"
	help "get the name (use -S to set) from the unit"
}
boolean 'R' {
	named "fSearch"
	help "request a search of the unit"
}

every {
	update "RcdsClient(%a);"
	param "hosts"
	help "send requests to these hosts"
}

%c
/* output the returned attribute list					(ksb)
 */
void
AttrDump(char *pcPrefix, char **ppcList)
{
	if ((char **)0 == ppcList) {
		printf("is (char **)0\n");
		return;
	}
	if ((char *)0 == pcPrefix) {
		pcPrefix = "\t";
	}
	while ((char *)0 != *ppcList) {
		printf("%s%s\n", pcPrefix, *ppcList++);
	}
}

/* give the remote customer data service some calls for			(ksb)
 * a debug session
 */
void
RcdsClient(host)
char *host;
{
	register CLIENT *clnt;

	if ((char *)0 == pcName) {
		pcName = "name";
	}

	clnt = clnt_create(host, RCDSPROG, RCDSVERS, "tcp");
	if ((CLIENT *)0 == clnt) {
		clnt_pcreateerror(host);
		exit(1);
	}

	if (fNew) {
		register int  *result_2;
		auto exists_param  uuidrnew_1_arg;

		uuidrnew_1_arg.pcuuid = acUuid;
		result_2 = uuidrnew_1(&uuidrnew_1_arg, clnt);
		if ((int *)0 == result_2) {
			clnt_perror(clnt, "new call failed");
		} else {
			printf("%s new=%d\n", uuidrnew_1_arg.pcuuid, *result_2);
		}
	}
	if (fExists) {
		register int  *result_1;
		auto exists_param  uuidrexists_1_arg;

		uuidrexists_1_arg.pcuuid = acUuid;
		result_1 = uuidrexists_1(&uuidrexists_1_arg, clnt);
		if ((int *)0  == result_1) {
			clnt_perror(clnt, "exists call failed");
		} else {
			printf("%s exists=%d\n", uuidrexists_1_arg.pcuuid, *result_1);
		}
	}
	if (fUSearch) {
		register search_res *pSR;
		auto new_param  NPUSearch;

		NPUSearch.pcuuid = acUuid;
		NPUSearch.pcunit = "*";
		NPUSearch.pcopts = pcOpts;
		pSR = uuidrsearch_1(& NPUSearch, clnt);
		if ((search_res *)0 == pSR) {
			clnt_perror(clnt, "search units call failed");
		} else {
			printf("%s search units (%s), returns %d\n", acUuid, (char *)0 != pcOpts ? pcOpts : acNull, pSR->iret);
			AttrDump("\t", pSR->ppcfound);
		}
	}
	if (fNewUnit) {
		register int  *result_4;
		auto new_param  unitrnew_1_arg;

		unitrnew_1_arg.pcuuid = acUuid;
		unitrnew_1_arg.pcunit = pcUnit;
		unitrnew_1_arg.pcopts = pcOpts;
		result_4 = unitrnew_1(&unitrnew_1_arg, clnt);
		if ((int *)0 == result_4 ) {
			clnt_perror(clnt, "new unit call failed");
		} else {
			printf("uid %s, unit new %s returns %d\n", unitrnew_1_arg.pcuuid, unitrnew_1_arg.pcunit, *result_4);
		}
	}
	if (fIndex) {
		register Cargv  *result_3;
		auto exists_param  uuidrindex_1_arg;

		uuidrindex_1_arg.pcuuid = acUuid;
		result_3 = uuidrindex_1(&uuidrindex_1_arg, clnt);
		if ((Cargv *)0 == result_3 ) {
			clnt_perror(clnt, "index call failed");
		} else {
			printf("index %s:\n", uuidrindex_1_arg.pcuuid);
			AttrDump("\t", *result_3);
		}
	}
	if (fPut) {
		auto put_param  unitrput_1_arg;
		register put_res  *result_5;

		unitrput_1_arg.pcuuid = acUuid;
		unitrput_1_arg.pcunit = pcUnit;
		unitrput_1_arg.pcname = pcName;
		unitrput_1_arg.pcvalue = pcValue;
		result_5 = unitrput_1(&unitrput_1_arg, clnt);
		if ((put_res *)0 == result_5) {
			clnt_perror(clnt, "put call failed");
		} else {
			printf("uid %s, put to unit \"%s\" %s", acUuid, pcUnit, pcName);
			if ((char *)0 != pcValue) {
				printf("=%s", pcValue);
			}
			printf("\nreturn %d", result_5->iret);
			if ((char *)0 != result_5->pcold) {
				printf(", old=%s", result_5->pcold);
			}
			printf("\n");
		}
	} else if (fGet) {
		register put_res  *result_6;
		auto new_param  unitrget_1_arg;

		unitrget_1_arg.pcuuid = acUuid;
		unitrget_1_arg.pcunit = pcUnit;
		unitrget_1_arg.pcopts = pcName;
		result_6 = unitrget_1(&unitrget_1_arg, clnt);
		if ((put_res *)0 == result_6) {
			clnt_perror(clnt, "get call failed");
		} else {
			printf("get %s, unit %s, name %s: %d\n", acUuid, pcUnit, pcName, result_6->iret);
			if ((char *)0 == result_6->pcold) {
				printf("%s\n", acNull);
			} else {
				printf("\r%s\n", result_6->pcold);
			}
		}
	} else if (fGaveNVP) {
		fprintf(stderr, "%s: NVP given, but not Put or Get\n", progname);
	}
	if (fSearch) {
		register search_res  *result_7;
		auto new_param  unitrsearch_1_arg;

		unitrsearch_1_arg.pcuuid = acUuid;
		unitrsearch_1_arg.pcunit = pcUnit;
		unitrsearch_1_arg.pcopts = pcOpts;
		result_7 = unitrsearch_1(&unitrsearch_1_arg, clnt);
		if ((search_res *)0 == result_7) {
			clnt_perror(clnt, "search call failed");
		} else {
			printf("search %s, unit %s, opts=%s: %d\n", acUuid, pcUnit, pcOpts ? pcOpts : acNull, result_7->iret);
			AttrDump("\t", result_7->ppcfound);
		}
	}
	if (fInfo) {
		register put_res  *result_8;
		auto new_param  unitrinfo_1_arg;

		unitrinfo_1_arg.pcuuid = acUuid;
		unitrinfo_1_arg.pcunit = pcUnit;
		unitrinfo_1_arg.pcopts = pcOpts;
		result_8 = unitrinfo_1(&unitrinfo_1_arg, clnt);
		if ((put_res *)0 == result_8) {
			clnt_perror(clnt, "info call failed");
		} else {
			printf("info %s, unit %s, opts %s: %d\n", acUuid, pcUnit, pcOpts ? pcOpts : acNull, result_8->iret);
			if ((char *)0 == result_8->pcold) {
				printf("%s\n", acNull);
			} else {
				printf("\t%s\n", result_8->pcold);
			}
		}
	}
	if (fSync) {
		register int  *piRes;
		auto new_param  NPSync;

		NPSync.pcuuid = acUuid;
		NPSync.pcunit = pcUnit;
		NPSync.pcopts = pcOpts;
		piRes = unitrsync_1(&NPSync, clnt);
		if ((int *) 0 == piRes) {
			clnt_perror(clnt, "sync call failed");
		} else {
			printf("sync %s, returned %d\n", pcUnit, *piRes);
		}
	}
	clnt_destroy(clnt);
}
