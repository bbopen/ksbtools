/* $Id: init.h,v 3.4 2004/06/25 12:39:02 ksb Exp $
 */
extern void Init();
extern char acUid[];
extern char *real_name();
extern int iUserUid;

#define DOTS_NO		(0)	/* called from command line parser	*/
#define DOTS_YES	(1)	/* called interactively			*/

extern void drop(), charon(), user();
extern int root();

extern void ShowTombs(), ShowCrypts();

extern LE *pLEFilesList;
