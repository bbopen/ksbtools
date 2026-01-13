/*
 * $Id: project.h,v 4.1 1996/07/14 18:54:35 kb207252 Exp $
 */

typedef enum {
	DISABLE,		/* submissions off			*/
	ENABLE,			/* submissions on for project		*/
	PACKSD,			/* tar and compress project		*/
	INITIALIZE,		/* set up the intructors account	*/
	OUTPUT,			/* display status			*/
	QUIT,			/* stop using project, request del	*/
	REMOVE,			/* delete submissions for proj		*/
	GRADE,			/* scan directories for tar file	*/
	LATE,			/* enable late submissions		*/
	UNKNOWN			/* we were not told			*/
} WHAT;

extern WHAT What;		/* what am I doing			*/
