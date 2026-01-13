/* $Id: HowMany.c,v 8.2 2000/03/11 16:49:32 ksb Exp $
 *
 * I get asked a lot about the limit on the number of console lines
 * per process: "Why is MAXMEMB so low?"
 *	#define MAXMEMB		8
 *	#define MAXGRP		32
 * Because the default number of file descriptors that a UNIX process can
 * open is about 63 on most (pre-2000) system installs.
 *
 * The console server has to have 2 files open for each console line, one
 * for the device (/dev/ttya) and one for the log file (/var/console/sulaco).
 * Plus the std 3, plus libc glop (~4) leaves one for each client attached.
 * 63 - 3 - 4 - (8 * 2) = 40 
 *
 * We can retune to 25 and leave 6 open client connections -- which would
 * be OK if you knew that client connections were rare and you NEVER needed
 * to attach to all the clients in a group at the same time.  I don't know
 * why you have a console server running so I must tune for the worst case.
 *
 * If you get a small number you might try a ulimit command like:
 *	ulimit -n 2048
 * to bump the limit up, remember to add this to you system startup
 * sequence!  As a normal user I couldn't get Solaris to give me more
 * than 127 file descriptors (which make a safe MAXMEMB be about 32).
 *
 * Recompile the console server with a DEBUG=-DMAXMEMB=13 on the make
 * command line, for example.  Or edit "cons.h".
 */
#include <fcntl.h>
#include <unistd.h>

/* This suggests a tune based on 80% use with 1 client on each line.	(ksb)
 */
int
main(argc, argv)
int argc;
char **argv;
{
	register int iCur, iMax;

	iMax = 2;
	while (-1 != (iCur =  dup(1)))
		iMax = iCur;
	printf("I could open %d file descriptors\n", iMax);
	if (iMax > 0x7fff) {
		printf("Which might break select on some hosts\n");
	}
	printf("Recommend MAXMEMB no greater than %d\n", (8*(iMax - 3 - 4))/30);
	exit(0);
}
