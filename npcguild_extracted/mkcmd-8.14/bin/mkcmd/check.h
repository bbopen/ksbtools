/* $Id: check.h,v 8.1 1997/10/13 16:05:55 ksb Exp $
 * output the only code
 */
extern char *struniq(/* char * */);
extern void mkonly(/* FILE * */); 
extern void mkchks(/* FILE * */);
extern int FixOpts(/* */);
extern char *SynthExclude(/* OPTION * */);
extern OPTION *FindBuf(/* OPTION *, char * */);
