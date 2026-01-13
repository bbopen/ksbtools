/*@Version $Revision: 6.0 $@*/
/*@Explode@*/
/*
 * $Micekey: AV AE
 * $Id: avl.h,v 6.0 2000/07/30 15:57:11 ksb Exp $
 * @(#)
 *
 * These are the definitions for the avl-tree routines.
 * Kevin Braunsdorf & Scott Guthridge, Copyright 1987
 *
 * prerequisite includes: <av_config>.h
 */

/* ksb's TEST configuration follows
 */
#ifdef TEST
typedef struct AEnode {
	char *pchname;
	int inumber;
} AE_ELEMENT;
#define nilAE	((AE_ELEMENT *)0)
#define newAE()	((AE_ELEMENT *)malloc(sizeof(AE_ELEMENT)))

extern int AECmp();
extern int AEMerge();
extern int AEPrint();
extern void AEInit();
extern int AEFree();
#define AEDelete	AEFree
#define AESame		AEFree
#define AENotfound()	yyerror("node not found")
#define Squeak		yyerror

/*
 * tokens for the test scanner
 */
#define TEOF		-1
#define HELP		 0
#define AUTOON		 1
#define AUTOOFF		 2
#define INC		 3
#define DEC		 4
#define EXCHANGE	 5
#define MERGE		 6
#define IDENTIFIER	 7
#define PRINT		 8
#define VERBOSE		 9
#define DEL		10
#define LAT		11
#define CONSTRUCT	12
#define FREE		13
#define STATUS		14
#define BEGIN		15
#define END		16
typedef short int TOKEN;
#endif TEST


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
extern int AVInsert();
extern int AVDelete();

extern AE_ELEMENT *AVpAEInsert;
extern AVL *AVpAVInsert;

extern AVCARDINAL AVCount();
extern int AVVerify();
extern char AVErrNone[], AVErrLeft[], AVErrRight[], AVErrCenter[];

extern AVL *AVConstruct();
extern AVL **AVLat();
extern AVL *AVMerge();

extern void AVFree();
extern void AVPrint();
extern int AVScan();

extern int AVBegin();
extern AE_ELEMENT *AVStep();
extern AVL *AVEnd();
