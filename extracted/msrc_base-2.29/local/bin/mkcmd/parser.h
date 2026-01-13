/* $Id: parser.h,v 8.4 2009/10/14 16:18:03 ksb Exp $
 */
extern void Config(/* char *pcFile */);
extern void Gen(/* char *pcBaseName, char *pcManPage */), Cleanup();
extern short int fGetenv, fPSawEvery, fPSawEnvInit;

extern char 
	acOpenOpt[],	/* optional param marker in order'd lists	*/
	acCloseOpt[];	/* end optionals marker in order'd lists	*/

extern void Init(/* char */);
extern void GenMeta(/* FILE *, LIST * */);
extern int TypeRequire(/* pctype*/);
extern void AddBasePair(/* pcName, pcOptions */);
extern FILE *MakeDivert(/*char *, char */);
