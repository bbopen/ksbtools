/* $Id: mkcmd.h,v 8.7 2000/06/14 13:02:52 ksb Exp $
 * based on some ideas from "mkprog", this program generates a command
 * line option parser based on a prototype line
 * see the README file for usage of this program & the man page
 */
#define LPAREN	'('
#define RPAREN	')'
#define LGROUP	'{'
#define RGROUP	'}'

extern LIST
	LIComm,			/* list of comments (up top)		*/
	aLIExits[11],		/* user hooks before exit		*/
	aLIInits[11],		/* user hooks before before's before	*/
	aLIAfters[11],		/* user hooks after afters's pre params	*/
	LIExcl,			/* list of global exclude bundles	*/
	LISysIncl,		/* list of our include files		*/
	LIUserIncl;		/* user includes, added after all ours	*/
extern int
	fBasename,
	fMix,
	iWidth;		/* option col width for verbose help	*/
extern char
	*pchGetenv,
	*pchProto,
	*pchTempl,
	*pchMain,
	*pchTerse,
	*pchUsage,
	*pchVector,
	*pchProgName,
	**ppcBases,
	*pcKeyArg,
	*pcDefBaseOpt,
	acFinger[],
	acKeepVal[],
	acTrack[];


extern void
	mkdecl(),
	mkintro(/* FILE *, int, FILE *, LIST * */),
	mkusage(),
	mkonly(),
	mkchks(),
	mkmain(),
	mkdot_h(),
	mkman();

extern char ac_true[], ac_false[];
#define BOOL_STR(Mb)	((Mb) ? ac_true : ac_false)
