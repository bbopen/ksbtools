/* $Id: routine.h,v 8.2 1997/10/18 17:35:34 ksb Exp $
 */

typedef struct RLnode {
	struct RLnode *pRLnext;	/* %mx next hunk			*/
	char **ppclorder;	/* %m{n} cols in order			*/
	char *pchelp;		/* %mh help/title for this hunk		*/
	char *pcbefore;		/* %mb before hunk			*/
	char *pcnamed;		/* %mn function to call (each line)	*/
	char *pcupdate;		/* %mc update action if not %Hn(%a);	*/
	char *pcuser;		/* %mu user code after update		*/
	char *pcaborts;		/* %me C code if line is too short	*/
	char *pcafter;		/* %ma after hunk			*/
	char *pcemark;		/* %mt token to terminate hunk		*/
	char *pcopenc;		/* %m# comment open delimiter (col1)	*/

	/* gen clues only -- converted by the check function
	 */ 
	unsigned int icols;	/* %m. number of cols + delims		*/
	char *pcleader;		/* %m. skip leading text (like tabs)	*/
	struct ORnode **ppORlorder;
				/* %m. conversion buffers directly	*/
	char **ppcdelims;	/* %m. ppORlorder[0] ppcdelims[0]...	*/
	char *pcargs;		/* %ml default arg list (cols in order) */
	char *pcpargs;		/* %mL default param list (cols in order)*/
	int imaxpwidth;		/* %m. param printf width for display	*/
	int ufixwidth;		/* %m. width of buffer for fix width in	*/
} R_LINE;

#define ROUTINE_STATIC	0x0001	/* routine should be static to prog.c	*/
#define ROUTINE_LOCAL	0x0002	/* hide vector/terse w/i usage()	*/
#define ROUTINE_CHECKED	0x0010	/* routine has been checked for sanity	*/
#define ROUTINE_FIXED	0x0020	/* routine has fixed width input	*/
#define ROUTINE_GEN	0x0100	/* routine has had code emitted for it	*/
#define ROUTINE_FMT	0x0200	/* routine has formated data checks	*/

typedef struct RGnode {
	int irattr;		/* routine attributes (static)		*/
	char *pcfunc;		/* %mI function name			*/
	char *pcraw;		/* %mG raw line read			*/
	char *pcprototype;	/* %mP emit to build hunk decode names	*/
	char *pcusage;		/* %mU a function to output usage	*/
	char *pcvector;		/* %mV help vector for hunk		*/
	char *pcterse;		/* %mT a char* variable of hunks	*/
	char *pclineno;		/* %mN line number buffer		*/
	KEY *pKV;		/* %mK unnamed API key			*/
	R_LINE *pRLfirst;	/* %M_ list of lines			*/

	/* gen and build clues
	 */
	struct ORnode *pORfunc;	/* %mi pcfunc mapped to an option rec	*/
	R_LINE **ppRL;		/* %m_ current line pointer		*/
	OPTION *pORraw;		/* %mg raw line read as buffer		*/
	unsigned uhunks;	/* %mj total hunks in file		*/
	unsigned umaxfixed;	/* %m. buffer space for fixed input cvt */
	char *pcarg1;		/* %mp first parameter: trigger type	*/
} ROUTINE;

#define RNT_STATIC(MpRG)	(0 != ((MpRG)->irattr & ROUTINE_STATIC))
#define RNT_LOCAL(MpRG)		(0 != ((MpRG)->irattr & ROUTINE_LOCAL))
#define RNT_CHECKED(MpRG)	(0 != ((MpRG)->irattr & ROUTINE_CHECKED))
#define RNT_FIXED(MpRG)		(0 != ((MpRG)->irattr & ROUTINE_FIXED))
#define RNT_GEN(MpRG)		(0 != ((MpRG)->irattr & ROUTINE_GEN))
#define RNT_FMT(MpRG)		(0 != ((MpRG)->irattr & ROUTINE_FMT))

extern struct EHnode EHRoutine;

extern int RLineEsc(/* pOR, ppRL, ppcEsc, pEO, pcOut */);
extern R_LINE *RLine(/* ppRL */);
extern int CheckRoutine(/* pOR */);
extern void mkreaders(/* FILE * */);
extern void RLineMan(/* FILE * */);
extern char *RColWhich(/* OPTION *, char * */);
extern char *RHunkWhich(/* OPTION *, char * */);
