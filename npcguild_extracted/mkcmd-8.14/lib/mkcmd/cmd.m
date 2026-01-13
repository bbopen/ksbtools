# $Id: cmd.m,v 8.9 1999/06/12 00:43:58 ksb Exp $
# these routines implement the `ksb std micro shell'			(ksb)
#
# This tiny shell is the thing `dbx', `lpc', `unrm', `ftp',
# `tftp', or maybe a mail reader would use to give you a chance
# to enter detailed commands.
#
# You may include ``cmd_source.m'' to provide the code for a source
# command, and ``cmd_help.m'' for a help command.  Then include
# CMD_DEF_COMMANDS, CMD_DEF_HELP and/or CMD_DEF_SOURCE
# in your command set table.  Others are in the manual page.
#
from '<stdio.h>'
from '<ctype.h>'

require "util_errno.m"
require "cmd.mc" "cmd.mi"

# force the code for envopt into the program to share
getenv

%hi
/* included here for separate compilation, need not be included in	(ksb)
 * the prog.c file
 */
#if !defined(CMD_NULL)
#define CMD_NULL	0x00	/* take no special action		*/
#define CMD_RET		0x01	/* return from the parser		*/
#define CMD_OFF		0x02	/* this command is `off' (priv?)	*/
#define CMD_HIDDEN	0x04	/* this command not in commands list	*/
#define CMD_NO_FREE	0x10	/* do not free argument vector		*/

typedef struct CMnode {
	char *pccmd;		/* command name, "quit"			*/
	char *pcdescr;		/* command help text, "terminate this"	*/
	int (*pfi)();		/* function to call		*/
	int facts;		/* action to take after function call	*/
	char *pcparams;		/* an optional usage message for params	*/
} CMD;

typedef struct CSnode {
	char *pcprompt;		/* the string to prompt with		*/
	unsigned int icmds;	/* how many commands we have		*/
	CMD *pCMcmds;	/* the list of commands to process	*/

	/* set via init */
	unsigned int ipad;	/* width for the command names		*/
	int *piambiguous;	/* number of chars user must provide	*/
} CMDSET;
#endif
%%

%h
extern void cmd_init();
extern int cmd_from_string();
extern int cmd_from_file();
extern int (*cmd_unknown)();
extern int (*cmd_ambiguous)();
extern int (*cmd_not_available)();
%%

%i
#define CMD_DEF_QUIT	{"quit", "exit this command interpreter", (int (*)())0, CMD_RET }
#define CMD_DEF_COMMENT	{"#", "a comment line", (int (*)())0, CMD_HIDDEN }
%%
