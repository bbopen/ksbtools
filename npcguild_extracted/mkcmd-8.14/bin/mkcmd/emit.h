/* $Id: emit.h,v 8.2 1997/11/01 19:18:04 ksb Exp $
 * req: type.h
 */

#define EMIT_NONE	0x0000	/* no special emit bits			*/
#define EMIT_QUOTE	0x0001	/* quote this for C strings		*/
#define EMIT_EXPAND	0x0010	/* re-expand the text I just sent you	*/
#define EMIT_AUTO_IND	0x0100	/* emulate cb/autoindent on output	*/
#define EMIT_DID_TABS	0x0200	/* someone else did the autoindent	*/

typedef struct EOnode {		/* emit options				*/
	ATTR weattr;		/* emit bits				*/
	int iindent;		/* %I? C code indentation level		*/
	char *pcarg1;		/* %a to the user			*/
	char *pcarg2;		/* %P for the generator			*/
	FILE *fp;		/* eventual output to this file		*/
} EMIT_OPTS;

#define EA_QUOTE(MpEO)	(0 != ((MpEO)->weattr & EMIT_QUOTE))
#define EA_EXPAND(MpEO)	(0 != ((MpEO)->weattr & EMIT_EXPAND))
#define EA_AUTO_IND(MpEO) (0 != ((MpEO)->weattr & EMIT_AUTO_IND))
#define EA_DID_TABS(MpEO) (0 != ((MpEO)->weattr & EMIT_DID_TABS))

extern ATTR wEmitMask;		/* global force emit options		*/

typedef struct ERnode {
	char cekey;		/* primative character			*/
	char *pcword;		/* long word to get primative op	*/
	char *pcdescr;		/* text to describe operation		*/
} EMIT_MAP;

#define ER_HIDDEN(MpER)	((char *)0 == (MpER)->pcdescr)

typedef struct EHnode {
	char *pclevel;		/* text to describe this level		*/
	char *pchow;		/* how we get to this expander		*/
	unsigned uhas;		/* how many functions we have		*/
	EMIT_MAP *pER;		/* the list of operations		*/
	char *pcnumber;		/* what to do on a number		*/
	struct EHnode
		*pEHcovers;	/* the operation list the hides up	*/

	/* internal hack for keys */
	unsigned ufound;	/* which of pER we found		*/
} EMIT_HELP;

#define EH_NUMBER(MpEH)	((char *)00 != (MpEH)->pcnumber)

extern char acNoOpt[], acCurOpt[], acEnvTemp[];
extern char *pcCurOpt;
extern char *pcJustEscape;

extern char *mkdefid(/*pOR*/);
extern char *mkid(/* OPTION *, char * */);

extern void EmitInit(/* char , int */);
extern void Emit(/* FILE *, char *, OPTION *, char *, int */);
extern void Emit2(/* FILE *, char *, OPTION *, char *, char *, int */);
extern void EmitN(/* FILE *, char *, OPTION *, int, int */);
extern int TopEsc(/* ppcCursor, pOR, pEO , pcDst */);
extern char *StrExpand(/* pcDo, pOR, pEO, uLen */);
extern void EmitHelp(/* FILE *, EMIT_HELP * */);
extern void MetaEmit(/* FILE *, OPTION *, FILE *, char * */);
extern char *CSpell(/* char[5], int, int */);
extern int EscCvt(/* EMIT_HELP *, char **, int * */);
extern EMIT_HELP EHTop;
