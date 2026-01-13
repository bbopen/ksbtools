/* $Id: findfiles.h,v 3.2 1994/07/11 00:16:29 ksb Exp $
 */
extern void
	CmdParse(),
	Interactive();

void FindFiles();

/*
 * whether to glob or not -- used as Boolean constants
 */
#define GLOB_NO		(0)
#define GLOB_YES	(1)

/* 
 * whether to break args or not -- used as Boolean constants
 */
#define BREAK_NO	(0)
#define BREAK_YES	(1)
#define BREAK_DONTCARE	(BREAK_NO)	/* for documentation purposes	*/
