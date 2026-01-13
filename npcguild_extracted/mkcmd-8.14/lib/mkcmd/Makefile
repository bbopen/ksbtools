#	$Id: Makefile,v 8.24 2000/07/28 13:57:38 ksb Exp $
#
#	Meta-Makefile for mkcmd lib

GEN=	
TEMPL=	Template.m bytes.m bytes.mi client.m \
bsd.m \
client.mi cmd.m cmd.mc cmd.mi cmd_add.m cmd_add.mc cmd_cd.m cmd_cd.mc \
cmd_dl.m cmd_dl.mc \
cmd_echo.m cmd_exec.m cmd_exec.mc cmd_exit.m cmd_help.m cmd_help.mc \
cmd_macro.m cmd_merge.m cmd_parse.m cmd_parse.mc cmd_shell.m \
cmd_shell.mc cmd_source.m cmd_source.mc cmd_umask.m cmd_umask.mc \
cmd_version.m date.m dir.m dir_check.m dir_check.mc example_cgi.m \
enum.m enum.mc enum.mh enum.mi enum.info \
example_cmd.m example_date.m example_dir.m example_dl.m \
example_dumpdates.m example_enum.m \
example_filter.m example_fts.m example_glob.m example_lex.m \
example_macro.m example_mult.m example_nettime.m example_ppm.m example_rc.m \
example_rls.m example_rls.x example_rlsd.m \
example_server.m example_time.m example_timebox.m glob_ieee.m \
filesystem.m filesystem.mc filesystem.mi \
getopt.m getopt_key.m getopt.mc getopt.mi shared.m shared.mi \
glob_ieee.mc hosttype.m hosttype.mh inches.m inches.mi \
rc.m seconds.m seconds.mi \
group.m group.mc login.m login.mc \
host.m host.mc host.mi protocol.m protocol.mc service.m service.mc \
rpc_client.m rpc_server.m rpc_server.mc \
server.m server.mi socket.m socket.mh socket.mi std_control.m \
std_datehdr.m std_filter.m std_fts.m std_help.m std_lex.m std_lex.mc \
std_macro.m std_macro.mc std_macro.mh std_macro.mi std_noargs.m \
std_targets.m std_version.m std_xdev.m time.m time_tz.m timebox.m \
tftp_client.m tftp_client.mc tftp_client.mh tftp_client.mi \
tftp_server.m tftp_server.mc tftp_server.mh tftp_server.mi \
util_by.m util_cgi.m util_cgi.mc util_client.m util_client.mc util_client.mi \
util_daemon.m util_daemon.mc util_daemon.mh \
util_date.m util_date.mc util_date.mi util_fgetln.m util_fgetln.mc \
util_dl.m util_dl.mc \
util_errno.m util_fts.m util_fts.mc util_glob.m \
util_editor.m util_editor.mc util_editor.mh util_editor.mi \
util_fsearch.m util_fsearch.mc util_fsearch.mh util_fsearch.mi \
util_getlogingr.m util_getlogingr.mi util_getlogingr.mc \
util_herror.m \
util_home.m util_home.mc util_home.mh util_home.mi \
util_mult.m util_mult.mc util_mult.mi \
util_ppm.m \
util_rc.m util_rc.mc util_rc.mi util_server.m \
util_savehostent.m util_savehostent.mc util_savehostent.mi \
util_savegrent.m util_savegrent.mc util_savegrent.mh util_savegrent.mi \
util_savepwent.m util_savepwent.mc util_savepwent.mh util_savepwent.mi \
util_saveservent.m util_saveservent.mc util_saveservent.mi \
util_sigret.m util_sigret.mi \
util_server.mc util_socket.m util_socket.mc util_socket.mi util_time.m \
util_tftp.m util_tftp.mh util_tftp.mi util_tftp.mc \
util_time.mc util_time.mh util_time.mi util_timebox.m util_timebox.mc \
util_timebox.mi util_tmp.m \
util_username.m util_username.mh util_username.mi util_username.mc \
util_unity.m util_unity.mh util_unity.mi util_unity.mc \
util_whittle.m util_zombie.m util_zombie.mc \
xdr_Cargv.m xdr_Cargv.mc xdr_Cargv.mh xdr_Cargv.mi \
xdr_Cstring.m xdr_Cstring.mc xdr_Cstring.mh xdr_Cstring.mi
SEND=	Make.host mkcmd.man TODO ${TEMPL}
SOURCE=	Makefile Distfile ${SEND}
INTO=	/usr/src/local/lib/mkcmd
HOSTS=	-S
MDEFS=	
DDEFS=	-dINTO=${INTO} ${MDEFS} ${HOSTS}


LOOP=	-for i in `distrib -H ${HOSTS}` ; do \
		echo $$i: ;\
		rsh $$i -n sh -c '". /usr/local/lib/distrib/local.defs && cd ${INTO} && ${MAKE} DESTDIR=${DESTDIR} DEBUG=${DEBUG} $@"' ;\
	done
HERE=	distrib -E -f Make.host -m `hostname` | ${MAKE} -f - $@

quit: FRC
	echo 1>&2 "You are in the wrong place to just do a make"

sanity: msource
	set -x ; TMPUU=/tmp/uus$$$$ ; \
	echo ${TEMPL} | tr ' ' '\012' >$$TMPUU ; \
	xapply "grep '^%1$$' >/dev/null $$TMPUU || echo %1" *.m *.mc *.mh *.mi ;\
	rm $$TMPUU

boot: rsource FRC
	${LOOP}

all: rsource FRC
	${LOOP}

calls: msource FRC
	${HERE}

clean: FRC
	${LOOP}
	rm -f ${GEN} Makefile.bak a.out core errs lint.out tags

deinstall: FRC
	${LOOP}

depend: FRC
	${LOOP}

dirs: FRC
	${LOOP}

distrib: FRC
	${LOOP}

install: rsource FRC
	${LOOP}

lint: FRC
	${LOOP}

mkcat: FRC
	${LOOP}

print: msource FRC
	${HERE}

source: rsource
	${LOOP}

rsource: Distfile msource
	distrib ${DDEFS}

msource: ${SOURCE}

spotless: clean FRC
	rcsclean ${SOURCE}

tags: msource FRC
	${HERE}

${SOURCE}:
	co -q $@

FRC:
