/*
 * $Id: filedup.h,v 8.0 1996/03/13 15:35:02 kb207252 Dist $
 * routines to keep track of dup files (hard links)			(ksb)
 */
typedef struct AE {
	dev_t mydev;
	ino_t myino;
	char *pcname;
	time_t kctime;		/* unlink and chmod are mean mod ctime	*/
} AE_ELEMENT;
#define nilAE	((AE_ELEMENT *)0)

/*
 * Indices for AVsbpAVchild tag
 */
#define AV_LCHILD	0
#define AV_RCHILD	1

/*
 * Balance factors (can be used to select child to traverse too)
 */
#define AV_BAL_LEFT	AV_LCHILD
#define AV_BAL_CENTER	-1
#define AV_BAL_RIGHT	AV_RCHILD

typedef unsigned AVCARDINAL;		/* short/normal/long unsigned 	*/

typedef struct AVnode {
	struct AVnode *AVsbpAVchild[2];	/* AVL children			*/
	short int AVbtipped;		/* AVL balance factor		*/
	AE_ELEMENT AE_data;
} AVL;
#define nilAV	((AVL *)0)
extern void AVInit();

typedef AVL *FILEDUPS;
extern char *FDAdd();
#define FDScan	AVScan
#define FDInit	AVInit
