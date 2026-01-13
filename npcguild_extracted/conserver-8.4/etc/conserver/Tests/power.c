/* $Id: power.c,v 8.1 1998/06/07 20:28:33 ksb Exp $
 * Put me on a virtual console to emulate a power controller. -- ksb
 *
 * We look like a Black Box POW-R-SWITCH 8c (or 4c).
 *
 * "Nothing stands between us here, and I won't be denied" -- Sarah
 */

#include <stdio.h>

/* we manage to look like the device in question to the console server
 * mostly because it puts a "\n" on all the commands.
 */
typedef struct PDnode {
	char ckey;		/* any code works, we use letters mostly */
	char cline;		/* '1' ... '8' only			 */
	char cpower;		/* '0' or '1'				 */
} POWER_DEVICE;


/* I'm not coding a read routine for this, not even in mkcmd just
 * to make your life easy.  A's start off, B's start on to show that
 * the state is random after a while.
 */
static POWER_DEVICE aPD[] = {
	{'A', '1', '0'}, {'A', '2', '0'}, {'A', '3', '0'},
	{'A', '4', '0'}, {'A', '5', '0'}, {'A', '6', '0'},
	{'A', '7', '0'}, {'A', '8', '0'},
	{'B', '1', '1'}, {'B', '2', '1'}, {'B', '3', '1'},
	{'B', '4', '1'}, {'B', '5', '1'}, {'B', '6', '1'},
	{'B', '7', '1'}, {'B', '6', '1'}
};

/* Act like the silly box, output mostly as it would.			(ksb)
 * This lets you test the POW-R-SWITCH mods w/o having one.
 *
 * We extend the device with "\n" to mean say "Arf!"  The client program
 * passes blank lines, that real commands and anything it doesn't grok.
 *
 * We should go into CBREAK mode and ignore EOF, but who cares?
 */
int
main(argc, argv)
int argc;
char **argv;
{
	auto char acIn[132];
	register int i;

	system("stty -echo");	/* what a cop out !!! */

	while (fgets(acIn, sizeof(acIn), stdin)) {
		if ('\n' == acIn[0]) {
			printf("Arf!");
			fflush(stdout);
			continue;
		}
		for (i = 0; i < sizeof(aPD)/sizeof(POWER_DEVICE); ++i) {
			if (acIn[0] != aPD[i].ckey) {
				continue;
			}
			if (acIn[1] != aPD[i].cline) {
				continue;
			}
			switch (acIn[2]) {
			case '#':
				break;
			case '0':
			case '1':
				aPD[i].cpower = acIn[2];
				break;
			default:
				/* yeah, say nothing */
				continue;
			}
			break;
		}
		if (i == sizeof(aPD)/sizeof(POWER_DEVICE)) {
			continue;
		}
		printf("%c%c%c", aPD[i].ckey, aPD[i].cline, aPD[i].cpower);
		fflush(stdout);
	}
	system("stty echo");	/* well at least we put it back */
	exit(0);
}
