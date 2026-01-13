/* mkcmd generated declarations for options and buffers
 */

extern char *progname, *au_terse[5], *u_help[23];
#ifndef u_terse
#define u_terse	(au_terse[0])
#endif
extern int main(), uGaveB, uGaveReq, fGaveMerge, fGavePara;
extern char *pcConfigs, *pcDebug, *pcMakefile, *pcHOST, *pcMakeHook, *pcXM4Define, *pcOutMerge, *pcExConfig, *pcZeroConfig, *pcTmpdir, *pcMakeClean;
extern int fLocalForced, fHXFlags, fShowMake, bHasRoot;
extern struct PPMnode *pPPMGuard, *pPPMM4Prep, *pPPMMakeOpt, *pPPMSpells;
/* from getopt.m */
/* from getopt_key.m */
/* from util_fgetln.m */
#line 9 "util_fgetln.m"
extern char *fgetln(/* FILE *, size_t * */);
/* from mmsrc.m */
/* from util_errno.m */
/* from util_ppm.m */
#line 11 "util_ppm.m"
#if !defined(UTIL_PPM_ONCE)
#define UTIL_PPM_ONCE	1
typedef struct PPMnode {
	void *pvbase;		/* present start			*/
	unsigned int uimax;	/* max we can hold now			*/
	unsigned int uiwide;	/* how wide is one element?		*/
	unsigned int uiblob;	/* how many elements do we grow by?	*/
} PPM_BUF;

extern PPM_BUF *util_ppm_init(PPM_BUF *, unsigned int, unsigned int);
extern void *util_ppm_size(PPM_BUF *, unsigned int);
extern void util_ppm_free(PPM_BUF *);
extern int util_ppm_print(PPM_BUF *, FILE *);
#endif
/* from std_help.m */
/* from std_version.m */
/* from evector.m */
#line 7 "evector.m"
#if !defined(EVECTOR_H)
/* A vector of these represents our macro defines, they are in the
 * order defined, because m4 macros are order dependent. Last name is NULL.
 * I know a lexical sort would make updates faster, that's not reason
 * enough to break the order semanitics here -- ksb.
 * We depend on ppm from mkcmd, of course.  And we leak memory like we are
 * inside a CGI!  This is not mean for a long running application.  -- ksb
 */
typedef struct MDnode {
	char *pcname, *pcvalue;		/* name defined as value	*/
} MACRO_DEF;

extern int ScanMacros(MACRO_DEF *, int (*)(char *, char *, void *), void *);
extern void BindMacro(char *, char *);
extern MACRO_DEF *CurrentMacros();
extern char *MacroValue(char *, MACRO_DEF *);
extern void ResetMacros();
extern MACRO_DEF *MergeMacros(MACRO_DEF *, MACRO_DEF *);
#define EVECTOR_H	1
#endif
/* from hostdb.m */
#line 68 "hostdb.m"
typedef struct MHnode {
	unsigned udefs;		/* number of times we were defined	*/
	unsigned iline;		/* line number, also "boolean forbids"	*/
	char *pcfrom;		/* config file we were first mentioned	*/
	char *pcrecent;		/* most recently mentioned file		*/
	char *pchost;		/* our host name from HOST		*/
	int uu;			/* the value we think %u has for the cmd*/
	int istatus;		/* exit code from xclate, under -r|-K	*/
	MACRO_DEF *pMD;		/* our macro definition list		*/
	struct MHnode *pMHnext;	/* traverse active hosts in order	*/
} META_HOST;
/* from slot.m */
/* from envfrom.m */
#line 13 "envfrom.m"
extern char *EnvFromFile(char **ppcList, unsigned uCount);
/* from make.m */
#line 46 "make.m"
/* The generic pull macros from a makefile interface
 */
typedef struct MVnode {
	char *pcname;			/* ${name} from the Makefile	*/
	unsigned int flocks;		/* never/always add to this	*/
	unsigned int fsynth;		/* we build from debris left	*/
	unsigned int ucur;		/* number of values found	*/
	/* opaque part */
	PPM_BUF PPMlist;		/* accumulated names		*/
	int afdpipe[2];			/* the result pipe we use	*/
} MAKE_VAR;

/* The specific interface for msrc and mmsrc (makeme2), this is the
 * booty we plunder from the makefile.
 */
typedef struct BOnode {
	char *pcinto;			/* INTO from the -y|makefile	*/
	char *pcmode;			/* MODE from the -y|makefile	*/
	char **ppcsubdir;		/* subdirs from SUBDIR		*/
	char **ppcmap;			/* target names from MAP	*/
	char **ppcorig;			/* source files	from MAP	*/
	char **ppcsend;			/* source files from SEND	*/
	char **ppctarg;			/* target names from SEND	*/
	char **ppcignore;		/* IGNORE + MAP's		*/
	char **ppchxinclude;		/* options to hxmd, viz. db	*/
	unsigned usubdir, umap, uorig, usend, utarg,
		uignore, uhxinclude;	/* counts of the above		*/
	unsigned usplay;		/* count of send != targ elems	*/
} BOOTY;
/* from util_fsearch.m */
/* from util_tmp.m */
/* from util_home.m */
/* from argv.m */

#line 1 "util_fsearch.mh"
/* $Id: util_fsearch.mh,v 8.10 1998/09/16 13:37:42 ksb Exp $
 */
extern char *util_fsearch();
#line 1 "util_home.mh"
/* $Id: util_home.mh,v 8.3 2004/12/15 22:20:18 ksb Exp $
 */
extern char *util_home();
