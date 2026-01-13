/* $Id: init.h,v 3.3 1994/07/11 00:15:34 ksb Exp $
 */
extern void Init();
extern char acUid[];
extern char *real_name();
extern int iUserUid;

#define DOTS_NO		(0)	/* called from command line parser	*/
#define DOTS_YES	(1)	/* called interactively			*/

#ifndef dirfd
#define dirfd(dirp)	((dirp)->dd_fd)
#endif

extern void drop(), charon(), user();
extern int root();

extern void ShowTombs(), ShowCrypts();

extern LE *pLEFilesList;
