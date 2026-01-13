/* $Id: rcds.x,v 2.2 2000/06/05 23:47:07 ksb Exp $
 *
 * Remote Customer Data Services -- provide a remote www customer a	(ksb)
 * simple data repository of arbitrary shape and size.
 */

struct exists_param {
	Cstring pcuuid;
};

struct new_param {
	Cstring pcuuid;
	Cstring pcunit;
	Cstring pcopts;
};

struct put_param {
	Cstring pcuuid;
	Cstring pcunit;
	Cstring pcname;
	Cstring pcvalue;
};
struct put_res {
	int iret;
	Cstring pcold;
};
struct search_res {
	int iret;
	Cargv ppcfound;
};

program RCDSPROG {
	version RCDSVERS {
		int
		uuidrexists(exists_param) = 1;

		int
		uuidrnew(exists_param) = 2;

		Cargv
		uuidrindex(exists_param) = 3;

		search_res
		uuidrsearch(new_param) = 4;

		int
		unitrnew(new_param) = 10;

		put_res
		unitrput(put_param) = 11;

		put_res
		unitrget(new_param) = 12;

		search_res
		unitrsearch(new_param) = 13;

		put_res
		unitrinfo(new_param) = 14;

		int
		unitrsync(new_param) = 15;
	} = 1;
} = 307352;
