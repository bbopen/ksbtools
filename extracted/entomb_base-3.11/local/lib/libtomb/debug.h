/* $Id: debug.h,v 3.0 1992/04/22 13:23:28 ksb Stab $
 * define if DEBUG is not defined, trace statements go away.
 *
 * Matthew Bradburn, Purdue University Computing Center
 */

extern void Trace();

/*
 * TRACE -- if DEBUG is defined, output a line to the trace file.
 * 	Use like "TRACE(("stuff %s no-newline"), pch);"  A newline
 *	is appended.
 */
#ifdef DEBUG
#define TRACE		Trace
#else
#define TRACE		(void)sizeof
#endif	/* DEBUG */

/*
 * directory for the trace output
 */
#define TRACE_PATH	("/tmp")
