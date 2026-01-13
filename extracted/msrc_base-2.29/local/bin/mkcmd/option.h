/* $Id: option.h,v 8.9 1999/04/21 20:29:06 ksb Exp $
 * based on some ideas from "mkprog", this program generates a command
 * line option parser based on a prototype line
 * see the README file for usage of this program & the man page
 */
#define MAXOPTS	256		/* max -letter options we could have	*/

/*
 * this type tells us everything about any option letter
 */
#define OPT_BREAK	0x000001 /* breaks option parsing when given	*/
#define OPT_ENDS	0x000002 /* returns from option parsing (after)	*/
#define OPT_ABORT	0x000004 /* abort program when given		*/
#define OPT_ONLY	0x000008 /* only given once, else error		*/
#define OPT_FORBID	0x000010 /* forbids other options		*/
#define OPT_STRICT	0x000020 /* forbidden by another option		*/
#define OPT_ENABLE	0x000040 /* enables a nested option (or more)	*/
#define OPT_ALTUSAGE	0x000080 /* option has different every or list	*/
#define OPT_GLOBAL	0x000100 /* this option is local to "main"	*/
#define OPT_LOCAL	0x000000 /* to ease my brain			*/
#define OPT_HIDDEN	0x000200 /* this option is hidden from the user	*/
#define OPT_STATIC	0x000400 /* opt var is static to the gen'd .c	*/
#define OPT_SACT	0x000800 /* a "control point", e.g. every	*/
#define OPT_VERIFY	0x001000 /* check for valid data		*/
#define OPT_PPARAM	0x002000 /* this option is a positional param	*/
#define OPT_DUPPARAM	0x004000 /* positional param was in this line	*/
#define OPT_VARIABLE	0x008000 /* this option is a user variable	*/
#define OPT_OOPEN	0x010000 /* option opens a left optional list	*/
#define OPT_OCLOSE	0x020000 /* option closes its left optional list*/
#define OPT_NOUSAGE	0x040000 /* no usage output			*/
#define OPT_NOGETOPT	0x080000 /* not passed to getopt as a letter	*/
#define OPT_INITMASK	0x300000 /* and with me to get INIT type	*/
#define  OPT_INITCONST	0x000000 /* just a constant expression		*/
#define  OPT_INITENV	0x100000 /* read $<pchinit> from the environ	*/
#define  OPT_INITEXPR	0x200000 /* runtime function call or whatnot	*/
#define OPT_TRACK	0x400000 /* `u_gave_i' is tracked for this opt	*/
#define OPT_TRGLOBAL	0x800000 /* the tracking buffer is global	*/
#define OPT_AUGMENT    0x1000000 /* add attributes to existing option	*/
#define OPT_ALIAS      0x2000000 /* is an alias				*/
#define OPT_LATE       0x4000000 /* is converted late by impl. req.	*/
#define OPT_PTR2       0x8000000 /* option is a dynamic array of type	*/
#define OPT_PIN	      0x10000000 /* this option lacks the enabler	*/
#define OPT_SYNTHTYPE 0x20000000 /* option is a user defined type	*/

#define OPT_UNIQUE	-2	/* a unique option letter for newopt	*/

#define GEN_INOPT	0x0001	/* this is a member of a [left bunch]	*/
#define GEN_OPTIONAL	0x0002	/* this parameter is shown with [these]	*/
#define GEN_PPPARAM	0x0004	/* posible positional param in this list*/
#define GEN_FAKEHIDE	0x0008	/* this option is temp. hidden		*/
#define GEN_EKEEP	0x0010	/* did extern for keep variable		*/
#define GEN_EDECL	0x0020	/* we emited (shouldn't emit) an extern	*/
#define GEN_SEEN	0x0100	/* we emited a usage line with this opt	*/
#define GEN_CNTL	0x0200	/* we emited a control stanza		*/
#define GEN_BAKED	0x0400	/* we did the type fix for this		*/
#define GEN_BUNDLE	0x1000	/* option is in a (visible) bundle	*/
#define GEN_PERMUTE	0x2000	/* option has been treated and ~enable	*/


typedef struct ORnode {
	int chname;		/* %l name of option			*/
	ATTR oattr;		/*    options on this option		*/
	char *pchdesc;		/* %p help message description		*/
	char *pchverb;		/* %h verbose help message		*/
	char *pchinit;		/* %i default value (if given)		*/
	char *pchname;		/* %n user given name			*/
	char *pchkeep;		/* %N variable to keep unconverted value*/
	char *pcnoparam;	/* %Z. update if no optional parameter	*/
	char *pchuupdate;	/* %c type update or user over ride	*/
	char *pchuser;		/* %u user given for this variable	*/
	char *pchtrack;		/* %U the `u_gave_i' flag the user wants*/
	OPTTYPE *pOTtype;	/* %x. option type			*/
	struct ORnode *pORnext;	/*    next in list			*/
	struct ORnode *pORalias;/*    aliases for option		*/
	char *pchverify;	/* %Zv function to call to verify a param*/
	char *pcdim;		/* %d if we have an EXPR for a dim	*/
	char *pcends;		/* %Ze code to `end' the command line here*/
	struct ORnode *pORali;	/* %A who we alias			*/
	struct ORnode *pORallow;/* %C option that contains us		*/
	struct ORnode *pORsact;	/* %R every/list/left/right for us only	*/
	char *pchbefore;	/* %Zb some initial action before opt proc*/
	char *pchafter;		/* %Za action to take after option proc	*/
	char *pchexit;		/* %ZE OPT_ABORT exit value		*/
	char *pchforbid;	/* %f options we forbid			*/
	char *pchfrom;		/* %F file to include for this action	*/
	unsigned iorder;	/* %. count of the positional slots	*/
	struct ORnode **ppORorder;/* %<D> old ppORLeft and ppORRight	*/
	char **ppcorder;	/*    old ppcLeft, ppcRight		*/
	struct RGnode *pRG;	/* %m/%M input routine description	*/
	struct KVnode *pKV;	/* %K keys for this option		*/

	/* type stuff */
	struct KVnode *pKVclient; /*  clients of this type node		*/
	char *pctmannote;	/* type manual page note		*/
	char *pctupdate;	/* type update				*/
	char *pctevery;		/* type every				*/
	char *pctdef;		/* type default value			*/
	char *pcthelp;		/* type default verbose help text	*/
	char *pctarg;		/* type param mnemonic			*/
	char *pctchk;		/* type check code			*/

	/* code gen stuff */
	ATTR gattr;		/*    code geration flags		*/
	char *pchgen;		/* %g used as an internal buffer	*/
	int ibundle;		/* %D count of justified params in a [..]*/
} OPTION;
#define nilOR	((OPTION *) 0)
#define newOR()	((OPTION *) malloc(sizeof(OPTION)))

#define ISBREAK(MpOR)	(0 != (((MpOR)->oattr) & OPT_BREAK))
#define ISENDS(MpOR)	(0 != (((MpOR)->oattr) & OPT_ENDS))
#define ISABORT(MpOR)	(0 != (((MpOR)->oattr) & OPT_ABORT))
#define ISONLY(MpOR)	(0 != (((MpOR)->oattr) & OPT_ONLY))
#define ISSTRICT(MpOR)	(0 != (((MpOR)->oattr) & OPT_STRICT))
#define ISFORBID(MpOR)	(0 != (((MpOR)->oattr) & OPT_FORBID))
#define ISENABLE(MpOR)	(0 != (((MpOR)->oattr) & OPT_ENABLE))
#define ISALTUSAGE(MpOR) (0 != (((MpOR)->oattr) & OPT_ALTUSAGE))
#define ISTRACK(MpOR)	(0 != (((MpOR)->oattr) & OPT_TRACK))
#define ISTRGLOBAL(MpOR)	(0 != (((MpOR)->oattr) & OPT_TRGLOBAL))
#define ISVERIFY(MpOR)	(0 != (((MpOR)->oattr) & OPT_VERIFY))
#define ISALIAS(MpOR)	(0 != (((MpOR)->oattr) & OPT_ALIAS))
#define ISGLOBAL(MpOR)	(0 != (((MpOR)->oattr) & OPT_GLOBAL))
#define ISLOCAL(MpOR)	(0 == (((MpOR)->oattr) & OPT_GLOBAL))
#define ISSTATIC(MpOR)	(0 != (((MpOR)->oattr) & OPT_STATIC))
#define ISSACT(MpOR)	(0 != (((MpOR)->oattr) & OPT_SACT))
#define ISPPARAM(MpOR)	(0 != (((MpOR)->oattr) & (OPT_PPARAM)))
#define ISDUPPARAM(MpOR) (0 != (((MpOR)->oattr) & (OPT_DUPPARAM)))
#define ISOOPEN(MpOR)	(0 != (((MpOR)->oattr) & (OPT_OOPEN)))
#define ISOCLOSE(MpOR)	(0 != (((MpOR)->oattr) & (OPT_OCLOSE)))
#define ISVARIABLE(MpOR) (0 != (((MpOR)->oattr) & (OPT_VARIABLE)))
#define ISALLOWED(MpOR)	 (nilOR != (MpOR)->pORallow)
#define UNALIAS(MMpOR)	(ISALIAS(MMpOR) ? (MMpOR)->pORali : (MMpOR))
#define ISHIDDEN(MpOR)	(0 != (((MpOR)->oattr) & (OPT_HIDDEN)))
#define ISVISIBLE(MpOR)	(0 == (((MpOR)->oattr) & (OPT_NOUSAGE|OPT_HIDDEN|OPT_PIN)))
#define ISNOUSAGE(MpOR)	(0 != (((MpOR)->oattr) & OPT_NOUSAGE))
#define ISNOGETOPT(MpOR) (0 != (((MpOR)->oattr) & OPT_NOGETOPT))
#define ISWHATINIT(MpOR)	((MpOR)->oattr & OPT_INITMASK)
#define ISAUGMENT(MpOR) (0 != (((MpOR)->oattr) & OPT_AUGMENT))
#define ISLATE(MpOR)	(0 != (((MpOR)->oattr) & OPT_LATE) || IS_LATEC((MpOR)->pOTtype))
#define ISPTR2(MpOR)	(0 != (((MpOR)->oattr) & OPT_PTR2))
#define ISSYNTHTYPE(MpOR) (0 != (((MpOR)->oattr) & (OPT_SYNTHTYPE)))

#define OPACTION(MpOR)	IS_ACT(UNALIAS(MpOR)->pOTtype)
#define ISTOGGLE(MpOR)	(sbTCopy == UNALIAS(MpOR)->pOTtype->pchupdate)

#define DIDFAKEHIDE(MpOR)	(0 != ((MpOR)->gattr & GEN_FAKEHIDE))
#define DIDINOPT(MpOR)		(0 != ((MpOR)->gattr & GEN_INOPT))
#define DIDOPTIONAL(MpOR)	(0 != ((MpOR)->gattr & GEN_OPTIONAL))
#define DIDPPPARAM(MpOR)	(0 != ((MpOR)->gattr & GEN_PPPARAM))
#define DIDEKEEP(MpOR)		(0 != ((MpOR)->gattr & GEN_EKEEP))
#define DIDEDECL(MpOR)		(0 != ((MpOR)->gattr & GEN_EDECL))
#define DIDEMIT(MpOR)		(0 != ((MpOR)->gattr & GEN_SEEN))
#define DIDCNTL(MpOR)		(0 != ((MpOR)->gattr & GEN_CNTL))
#define DIDBAKE(MpOR)		(0 != ((MpOR)->gattr & GEN_BAKED))
#define DIDBUNDLE(MpOR)		(0 != ((MpOR)->gattr & GEN_BUNDLE))
#define DIDPERMUTE(MpOR)	(0 != ((MpOR)->gattr & GEN_PERMUTE))

#define CHDECL	'.'		/* a global variable			*/
#define DEFCH	'['		/* default case in switch ]		*/

extern OPTION
	*pORActn,	/* special actions in program		*/
	*pORDecl,	/* globals we are to declare		*/
	*pORRoot,	/* program's options			*/
	*pORType;	/* library defined types		*/
extern OPTION *newopt();
extern OPTION *FindVar();
extern OPTION *OptScanAll(/* pi, pOR */);
extern char *CaseName(/* pOR */);
extern struct EHnode EHOpt;
extern OPTION *OptParse(/* char **, OPTION * */);
extern int OptEsc(/* char **, OPTION *, EMIT_OPTS *, char * */);

extern struct EHnode EHSact;
extern OPTION *SactParse(/* char **, OPTION *, int * */);
extern char *sactstr(/* int */);
extern char *usersees(/* OPTION *, char * */);
extern void BuiltinTypes();
