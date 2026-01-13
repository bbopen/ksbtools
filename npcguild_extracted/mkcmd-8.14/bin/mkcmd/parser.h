/* $Id: parser.h,v 8.2 1997/10/13 16:05:55 ksb Exp $
 */
extern char *AddIncl();
extern void Config(/* char *pcFile */);
extern void Gen(/* char *pcBaseName, char *pcManPage */);
extern short int fGetenv, fPSawEvery, fPSawEnvInit;

extern char 
	acOpenOpt[],	/* optional param marker in order'd lists	*/
	acCloseOpt[];	/* end optionals marker in order'd lists	*/

extern void Init(/* char */);
extern void GenMeta(/* FILE *, LIST * */);
extern int TypeRequire( /* pctype*/);
extern void AddBasePair( /* pcName, pcOptions */);
