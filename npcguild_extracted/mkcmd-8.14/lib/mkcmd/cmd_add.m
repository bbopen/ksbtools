#!mkcmd
# $Id: cmd_add.m,v 8.7 1999/06/14 14:33:12 ksb Exp $
# An interface to add commands to make the CUI less captive		(ksb)

require "cmd_merge.m" "cmd_parse.m" "cmd_add.mc"

%i
static int cmd_add();
#define CMD_DEF_ADD {"add", "add executables to the interpreter", cmd_add, 0, "[files]"}
%%
