/* $Id: fslist.h,v 3.3 1998/02/12 19:57:43 ksb Exp $
 * struct type for dynamically allocated list of Tomb Information
 */
typedef struct TIStruct {
	char *pcDir;			/* path to tomb */
	char *pcDev;			/* device on which tomb resides */
	struct TIStruct *pTINext;
	char *pcFull;			/* the fullpath to the tomb	*/
	ino_t tino;			/* the tombs inode		*/
	dev_t tdev;			/* the tombs device number	*/
} TombInfo;

#if LINUX
#define NOT_A_DEV	((dev_t)0)
#else
#define NOT_A_DEV	(-1)
#endif

extern TombInfo *FsList();
extern TombInfo *TIDelete();
extern TombInfo *TINew();
