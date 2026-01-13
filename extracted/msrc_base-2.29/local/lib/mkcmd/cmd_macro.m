# $Id: cmd_macro.m,v 8.7 1997/10/27 11:55:22 ksb Exp $
# This provides an interface to change macros with the tiny-shell	(ksb)
# after cmd_init we install the hook for you.
#
%i
static int (*cmd_macro_slave)();
%%

%c
/* We accept macro=value or macro = value				(ksb)
 * or +macro, or -macro just like the command line
 * also ?macro will show the value.
 * We know about aMC being the root of the macros.
 */
static int
cmd_macro(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register struct MCnode *pMC;

	if (1 == argc) {
		if ((struct MCnode *)0 != (pMC = FindMacro(argv[0]))) {
			if ((char *)0 == pMC->u_pccur) {
				printf("%s=(null)\n", argv[0]);
			} else {
				printf("%s=%s\n", argv[0], pMC->u_pccur);
			}
			return 0;
		}
		if ((char *)0 != strchr(argv[0], '=')) {
			DoMacro(argv[0]);
			return 0;
		}
		switch (argv[0][0]) {
		case '-':
			DoMacro(argv[0]+1);
			return 0;
		case '+':
			UndoMacro(argv[0]+1);
			/* lost memory here XXX */
			return 0;
		case '/':
			ListMacros(stdout);
			/* lost memory here XXX */
			return 0;
		case '?':
			printf("macros access with:\nmacro=value set macro\n-macro\t    set to defalut\n+macro\t    reset macro\n/\t    list macros\n?\t    this help message\n");
			return 0;
		}
	} else if (3 == argc && '=' == argv[1][0] && '\000' == argv[1][1] && (struct MCnode *)0 != (pMC = FindMacro(argv[0]))) {
		pMC->u_pccur = argv[2];
		return 0;
	}
	return (*cmd_macro_slave)(argc, argv, pCS);
}
%%
init "/* make the macro trap the handler for unknown commands */"
init "cmd_macro_slave = cmd_unknown;"
init "cmd_unknown = cmd_macro;"
init "u_bit_unknown |= CMD_NO_FREE;"

key "macro_hooks" 1 {
	1
	"return;"
}

augment key "help_banner" 1 {
	'	printf("%%s: use \\"?\\" for macro help\\n", %b)%;'
}
