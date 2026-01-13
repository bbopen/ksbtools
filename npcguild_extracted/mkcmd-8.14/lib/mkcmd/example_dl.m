# $Id: example_dl.m,v 8.32 1999/09/25 20:09:15 ksb Exp $
# This is a little complicated to test, but it is way cool.		(ksb)
# We are going to make a new micro shell command to dynamically load a
# binary (compiled C or the like) command set into a running micro shell.
#
# We need to build a version of the example_cmd.m program that knows
# how to dynamically load files (this works on a Sun running 2.6):
#	$ mkcmd example_cmd.m cmd_dl.m
#	$ gcc -o cmd2 prog.c -ldl
#
# Then we need to build an example dynamiclly linked command:
#	$ mkcmd example_dl.m
#	$ gcc -fpic -c prog.c
#	$ ld -Bdynamic -G -o try.so prog.o
#
# Then show how it works:
#	$ ./cmd2
#	example> help vi
#	cmd2: vi: unknown command
#	example> dl ./try.so
#	example> help vi ok
# 	vi [files] edit a file
#	ok thumbs up
#	example> ok
#	aok!
#	example> quit
# You can replace "fodder" with you own C scroll and add as many as you
# like.
require "cmd.m" "cmd_exec.m" "util_dl.m"

%i
extern struct CMnode CMDynamic[];
%%

# By setting the key CMD_INFO[3] you can add an run-time init,
# be sure it returns true when all is well.  See also util_dl.m.
#key "CMD_INFO" 1 {
#	2 "my_init"
#}
%c
#if 0
/* sample init function							(ksb)
 * pv is the dlopen handle on this module.
 */
int
my_init(pv)
void *pv;
{
	write(1, "init\n", 5);
	return 1;
}
#endif
%%

%c
/* Just a silly command to show out dynamic link spell			(ksb)
 */
fodder(argc, argv)
int argc;
char **argv;
{
	write(1, "aok!\n", 5);
	return 0;
}

/* The definition of the commands we add to the micro shell,
 * note that we _call_back_ for cmd_fs_exec.
 */
CMD CMDynamic[] = {
	{"ok", "thumbs up", fodder, 0},
	{"vi\0/usr/bin/vi", "edit a file ", cmd_fs_exec, 0, "[files]"}
};
%%
