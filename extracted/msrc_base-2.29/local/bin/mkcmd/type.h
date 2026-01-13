/* $Id: type.h,v 8.7 1998/09/19 21:27:57 ksb Exp $
 * mkcmd's type abstraction
 */

typedef long ATTR;		/* 31 bits at least			*/
#if !defined(nil)
#define nil	((char *)0)
#endif

/* this type is keeps track of the "type" of all the options
 */
#define _ARG	 0x000000	/* this type is passive			*/
#define _ACT	 0x000001	/* the type is an action type		*/
#define _USED	 0x000002	/* this type is used			*/
#define _DONE	 0x000004	/* this type is done declare		*/
#define _JARG	 0x000008	/* a justified argument of this type	*/
#define _CHK	 0x000010	/* check a parameter			*/
#define _SYNTHX	 0x000020	/* busy user built type			*/
#define _SYNTH	 0x000040	/* user built type, from base		*/
#define _OPAQUE	 0x000080	/* new type is not a derived type	*/
#define _XBASE	 0x000100	/* type has a trigger specific base	*/
#define _ACTIVE	 0x000200	/* init cannot be statically given	*/
#define _REQVAL	 0x000400	/* we *must* have an initial value	*/
#define _MKEEP	 0x000800	/* manditory keep slot			*/
#define _LATEC	 0x001000	/* type has a late conversion		*/
#define _EWLATE	 0x002000	/* error when late			*/
#define _EWNOPR	 0x004000	/* error when optparam &~ noparam trap	*/
#define _VUSED	 0x008000	/* built-in type check accessed		*/
#define RTN_MASK 0x0f0000	/* mask for routine type		*/
#define _RTN_VD  0x010000	/* routine expects nothing		*/
#define _RTN_PC	 0x020000	/* routine expects to scan (char*)	*/
#define _RTN_FD	 0x040000	/* routine expects to read (fd)		*/
#define _RTN_FP	 0x050000	/* routine expects to read (FILE*)	*/

typedef struct OTnode {
	int chkey;		/* %k a type key			*/
	ATTR tattr;		/*    flags for any of this type, etc.	*/
	char *pchbase;		/* %b string for base type		*/
	char *pchxbase;		/* %B used as %XxB to get void* base	*/
	char *pchdecl;		/* %d usercall for definition		*/
	char *pchextn;		/* %a for dot-h extern declaration	*/
	char *pchdecl2;		/* %D define a pointer to one		*/
	char *pchextn2;		/* %A declare a pointer to one		*/
	char *pchupdate;	/* %u default string for update		*/
	char *pchevery;		/* %e default code for use as `every type'*/
	char *pchdef;		/* %i default initalizer		*/
	char *pcharg;		/* %p something to decsribe type	*/
	char *pchcext;		/* %x extern for std convert function	*/
	char *pchaext;		/* %x extern for ANSI convert function	*/
	char *pchchk;		/* %v function call to check valid value*/
	char *pchmannote;	/* %m usercall for man description	*/
	int (*pfifix)();	/*    per-type fix-up routine		*/
	char *pchlabel;		/* %l output label for table		*/
	char *pchhelp;		/* %h default help string (yucky)	*/
	struct ORnode *pORname;	/* %Z this types trigger/synth name	*/
	struct OTnode *pOTsbase;/* %s synthetic base type		*/
} OPTTYPE;
#define nilOT	((OPTTYPE *) 0)
#define IS_ACT(MpOT)	(0 != ((MpOT)->tattr & _ACT))
#define IS_USED(MpOT)	(0 != ((MpOT)->tattr & _USED))
#define IS_DONE(MpOT)	(0 != ((MpOT)->tattr & _DONE))
#define IS_JARG(MpOT)	(0 != ((MpOT)->tattr & _JARG))
#define IS_CHK(MpOT)	(0 != ((MpOT)->tattr & _CHK))
#define IS_SYNTH(MpOT)	(0 != ((MpOT)->tattr & _SYNTH))
#define IS_OPAQUE(MpOT)	(0 != ((MpOT)->tattr & _OPAQUE))
#define IS_XBASE(MpOT)	(0 != ((MpOT)->tattr & _XBASE))
#define IS_ACTIVE(MpOT)	(0 != ((MpOT)->tattr & _ACTIVE))
#define IS_REQVAL(MpOT)	(0 != ((MpOT)->tattr & _REQVAL))
#define IS_MKEEP(MpOT)	(0 != ((MpOT)->tattr & _MKEEP))
#define IS_LATEC(MpOT)	(0 != ((MpOT)->tattr & _LATEC))
#define IS_EWLATE(MpOT)	(0 != ((MpOT)->tattr & _EWLATE))
#define IS_EWNOPR(MpOT)	(0 != ((MpOT)->tattr & _EWNOPR))
#define IS_VUSED(MpOT)	(0 != ((MpOT)->tattr & _VUSED))
#define IS_NO_RTN(MpOT)	(0 == ((MpOT)->tattr & RTN_MASK))
#define IS_RTN_VD(MpOT)	(_RTN_VD == ((MpOT)->tattr & RTN_MASK))
#define IS_RTN_PC(MpOT)	(_RTN_PC == ((MpOT)->tattr & RTN_MASK))
#define IS_RTN_FD(MpOT)	(_RTN_FD == ((MpOT)->tattr & RTN_MASK))
#define IS_RTN_FP(MpOT)	(_RTN_FP == ((MpOT)->tattr & RTN_MASK))

typedef struct OInode {
	char *pciname;
	char cikey;
	char *pciinit;
	ATTR wiattr;
} OINIT;
extern OINIT aOIBases[];

/*
 * the "constants" are used throught the code to specail case some
 * usages in the generated C code
 */
extern char
	sbEmpty[],
	sbCBase[], sbIBase[], sbCXBase[], sbVXBase[],
	sbPInit[], sbBInit[], sbSInit[],
	sbTCopy[],
	sbGenFd[],
	sb1Call[], sb2Call[],
	sbIExt[], sbDExt[], sbLExt[],
	sbActn[], sbIChk[], sbDChk[], sbSChk[];

extern OPTTYPE OTAlias, aOTTypes[];
extern int TypeEsc(/* char **, OPTTYPE *, EMIT_OPTS * , char * */);
extern OPTTYPE *TypeParse(/* char ** */);
extern OPTTYPE *CvtType(/* int */);
extern OPTTYPE *SynthNew(/* pORType, pOTBase, pcName*/);
extern int SynthResolve(/* pORtype */);
extern void DumpTypes(/* FILE *fp */);
extern struct EHnode EHType, EHBase;
extern void mktypeext(/* FILE * */);
