/* mkcmd generated declarations for options and buffers
 */

extern char *progname, *au_terse[6], *u_help[15];
#ifndef u_terse
#define u_terse	(au_terse[0])
#endif
extern int main();
extern char *pcBootModule, *pchBaseName, *pcOutPlate, acDefTDir[], acOutMem[];
extern int fAnsi, fWeGuess, fKeyTable, fMaketd, fCppLines, fVerbose, iExit, fLanguage;
/* from getopt.m */
/* from mkcmd.m */
#line 9 "mkcmd.m"
/* $Id: main.h,v 8.6 1997/11/23 00:29:47 ksb Flux $
 */
#if !defined(MER_INV)
#define MER_INV		0x20	/* internal problem			*/
#define MER_SEMANTIC	0x10	/* strange idea, but we can't		*/
#define MER_LIMIT	0x08	/* internal limit problem		*/
#define MER_SYNTAX	0x04	/* syntax in input file			*/
#define MER_BROKEN	0x02	/* oush! what is this?			*/
#define MER_PAIN	0x01	/* we don't even know			*/
#endif

extern FILE *Search(/* char *pcFile, char *pcMode, int (**pfiClose)() */);
/* from std_version.m */
/* from std_help.m */
/* from scan.m */

#line 1 "mkcmd.mh"
/* $Id: main.h,v 8.6 1997/11/23 00:29:47 ksb Flux $
 */

/* mkcmd interface to produce unique tokens for mkcmd's scanner		(ksb)
 */
#define INTEGER (1-1)
#define SEMI (2-1)
#define STRING (3-1)
#define TABORTS (4-1)
#define TACCUM (5-1)
#define TACTION (6-1)
#define TAFTER (7-1)
#define TAUGMENT (8-1)
#define TBADOPT (9-1)
#define TBASENAME (10-1)
#define TBEFORE (11-1)
#define TBOOLEAN (12-1)
#define TCHARP (13-1)
#define TCLEANUP (14-1)
#define TCOMMENT (15-1)
#define TDOUBLE (16-1)
#define TDYNAMIC (17-1)
#define TENDS (18-1)
#define TESCAPE (19-1)
#define TEVERY (20-1)
#define TEXCLUDES (21-1)
#define TEXIT (22-1)
#define TFD (23-1)
#define TFILE (24-1)
#define TFROM (25-1)
#define TFUNCTION (26-1)
#define TGETENV (27-1)
#define TGLOBAL (28-1)
#define THELP (29-1)
#define THIDDEN (30-1)
#define TINITIALIZER (31-1)
#define TINTEGER (32-1)
#define TKEEP (33-1)
#define TKEY (34-1)
#define TLATE (35-1)
#define TLEFT (36-1)
#define TLETTER (37-1)
#define TLIST (38-1)
#define TLOCAL (39-1)
#define TLONG (40-1)
#define TMIX (41-1)
#define TNAMED (42-1)
#define TNOPARAM (43-1)
#define TNUMBER (44-1)
#define TONCE (45-1)
#define TOTHERWISE (46-1)
#define TPARAMETER (47-1)
#define TPOINTER (48-1)
#define TPROTOTYPE (49-1)
#define TREQUIRES (50-1)
#define TRIGHT (51-1)
#define TROUTINE (52-1)
#define TSTATIC (53-1)
#define TSTRING (54-1)
#define TSTOPS (55-1)
#define TTEMPLATE (56-1)
#define TTERSE (57-1)
#define TTOGGLE (58-1)
#define TTRACK (59-1)
#define TTYPE (60-1)
#define TUNSIGNED (61-1)
#define TUPDATE (62-1)
#define TUSER (63-1)
#define TUSAGE (64-1)
#define TVARIABLE (65-1)
#define TVECTOR (66-1)
#define TVERIFY (67-1)
#define TZERO (68-1)
#define LBRACKET (69-1)
#define RBRACKET (70-1)
#define LCURLY (71-1)
#define RCURLY (72-1)
#define TEOF (73-1)

