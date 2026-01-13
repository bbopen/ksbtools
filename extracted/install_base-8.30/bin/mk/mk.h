/* $Id: mk.h,v 5.3 2006/10/04 23:24:56 ksb Exp $
 * Override these in Make.host with -D in CDEFS, don't edit this file
 * unless you are a Real Wizard (tm).
 */
#define PATCH_LEVEL	0

/* This define controls the auxilary look up of compilation
 * directives in a standard template directory.	(%~)
 */
#if !defined(AUXDIR)
#define AUXDIR	"/usr/local/lib/mk"
#endif

/* This defines a prescan template (-e) to trap bad file types/names
 * (This is what keeps us from opening a .Z file and scanning it.)
 * %=//%x/%<%~/map/magic>
 */
#if !defined(BEFORE)
#define BEFORE	"%Yd%f/.mk-rules:%~/type/%y%;:%~/%<%~/map/pre>:%~/pre/%x"
#endif

/* This is the template to scan the file given on the command line.
 * (The user cannot change it (-F?), which is a `bug'.)
 */
#if !defined(FSCAN)
#define FSCAN	"%Yf%f"
#endif

/* We look at these teplates (-t) after exhausting 99 lines (-l) from the file.
 */
#if !defined(TEMPL)
#define TEMPL   "%~/%<%~/map/post>:%Yf%~/file/%G:%~/dot/%x:%~/m/%M"
#endif

/* The default marker (-m) should not be changed in English speaking
 * places.  If you don't grok English put in some clever verb in your
 * favorite LATIN1 text (multibyte characters will not work).
 */
#if !defined(DEFMARK)
#define DEFMARK	"Compile"
#endif

extern void Version();
extern int process(), define(), undefine();
extern char **AddClean(char *);
