/*
 * $Id: cons.h,v 8.1 1998/06/07 16:10:18 ksb Exp $
 *
 * Copyright 1992 Purdue Research Foundation, West Lafayette, Indiana
 * 47907.  All rights reserved.
 *
 * Written by Kevin S Braunsdorf, ksb@cc.purdue.edu, purdue!ksb
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. Neither the authors nor Purdue University are responsible for any
 *    consequences of the use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Credit to the authors and Purdue
 *    University must appear in documentation and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 */

/*
 * this is the configuration file for the Ohio State/PUCC console
 * server.  Just define the macros below to somehting that looks good
 * and give it a go.  It'll complain (under conserver -V) if things
 * look really bad.
 *
 * all PTX, PTX2, and PTX4 code added by gregf@sequent.com		(gregf)
 */

/* This is the port number used in the connection.  It can use either
 * /etc/services or a hardcoded port (SERVICE name has precedence).
 * (You can -D one in the Makefile to override these.)
 */
/* #define PORT 782 /* only if you cannot put in /etc/services */
#if !defined(SERVICE)
#if !defined(PORT)
#define SERVICE		"conserver"
#endif
#endif

/* The name of the host which will act as the console server
 */
#if !defined(HOST)
#define HOST		"console.cc.purdue.edu"
#endif

/* the default escape sequence used to give meta commands
 */
#define DEFATTN		'\005'
#define DEFESC		'c'

/* Location of the configuration file
 */
#if !defined(CONFIG)
#define CONFIG		"/usr/local/lib/conserver.cf"
#endif


/* The maximum number of serial lines that can be handled by a child process
 */
#if !defined(MAXMEMB)
#define MAXMEMB		8
#endif


/* The maximum number of child processes spawned.
 */
#if !defined(MAXGRP)
#define MAXGRP		32
#endif

/* the max number of characters conserver will replay for you (the r command)
 */
#if !defined(MAXREPLAY)
#define MAXREPLAY	(80*40)
#endif

/* the console server will provide a pseudo-device console which
 * allows operators to run backups and such without a hard wired
 * line (this is also good for testing the server to see if you
 * might wanna use it).  Turn this on only if you (might) need it.
 */
#if !defined(DO_VIRTUAL)
#define DO_VIRTUAL	1
#endif

/* Allows a console port to have an attached power controller.  I use the
 * Black Box Pow-R-Switch 8C (costs about $450 for 8 lines).
 * You can use anything that you can write a console interface for:
 *	some "token" (up to 6 characters) to indicate the line
 *	send token + "1\n" for on
 *	send token + "0\n" for off,
 *	send token + "#\n" for which (optional)
 *	send token + any-other-character + "\n" for your own extensions
 * Basically we tell the client to attach to that console port and send
 * the "token" + the command characters until it sees ".".  Then return to
 * us.  Easy.  A sample C program to put on a Virtual console would be
 * simple (to interface to more complex power controllers).
 * The power controller can be password protected separately from the
 * console line... of course.	-- ksb
 *
 * This extends the configuration line by two fields [:token:line@sever]
 * to get the token and connection info.  Default to "line" and none.
 */
#if !defined(DO_POWER)
#define DO_POWER	1
#endif

/* that's all.  just run
 *	make
 *	./conserver -V
 */

/* communication constants
 */
#define OB_POWER	'P'		/* call my power controller	*/
#define OB_SUSP		'Z'		/* suspended by server		*/
#define OB_DROP		'.'		/* dropped by server		*/

/* Due to C's poor man's macros the macro below would break if statements,
 * What we want
 *	macro()			{ stuff }
 * but the syntax gives us
 *	macro()			{ stuff };
 *
 * the extra semicolon breaks if statements!
 * Of course, the one we use makes lint scream:
 *	macro()			do { stuff } while (0)
 *
 * which is a statement and makes if statements safe
 */
#if defined(lint)
extern int shut_up_lint;
#else
#define shut_up_lint	0
#endif

/* this macro efficently outputs a constant string to a fd
 * of course it doesn't check the write :-(
 */
#define CSTROUT(Mfd, Mstr)	do {	\
	static char _ac[] = Mstr; \
	write(Mfd, _ac, sizeof(_ac)-1); \
	} while (shut_up_lint)
