/*
 * $Id: scan.h,v 3.2 1992/12/09 09:57:08 ksb Dist $
 *
 * scan.h -- scanner for calls
 *	<stdio> must be included before this file, and
 *	"main.h" is assumed the main.h defines BUCKET, also included before
 */

#define LCURLY	  '{'	        /*}*/	/* messes with vi's mind 	*/
#define RCURLY	 /*{*/ 	 	 '}'	/* to have curly in text	*/
#define LPAREN	  '('		/*)*/	/* same mess			*/
#define RPAREN   /*(*/		 ')'	/* as above			*/
#define LBRACK	  '['		/*]*/	/* more mess implies		*/
#define RBRACK	 /*[*/		 ']'	/* more mass			*/
#define BUCKET		100		/* number of objects to alloc	*/
#define MAXCHARS	80		/* max number of chars in ident	*/

#define SAVECLINE	0		/* save caller line		*/

typedef int LINENO;			/* type of a line number	*/

/*
 * data structure for a call or variable reference
 */
typedef struct INnode {
	struct HTnode *pHTname;		/* namep;			*/
	struct INnode *pINnext;		/* pnext			*/
	short int ffunc;		/* variable ref or function ref	*/
#if SAVECLINE
	LINENO icline;
#endif	/* save caller line		*/
} INST;
#define nilINST	((INST *) 0)

typedef struct HTnode {
	char *pcname, *pcfile;		/* name & file declared		*/
	struct HTnode *pHTnext;		/* next in table (list)		*/
	struct INnode *pINcalls;	/* list of calls		*/
	LINENO
		iline,			/* line output on		*/
		isrcline,		/* source line we found it on	*/
		isrcend;		/* source line we end on	*/
	short int
		listp,			/* 0 = don't, 1 = do, 2 = done	*/
		calledp,		/* have we ever been called	*/
		localp,			/* crude static function flag	*/
		intree,			/* is the node in the graph	*/
		inclp;			/* we are defined by inclusion	*/
} HASH, *PHT;
#define nilHASH	((HASH *) 0)

extern void Level1();
extern FILE *fpInput;
extern HASH *Search(), *pHTRoot[2];
extern INST *newINST();
