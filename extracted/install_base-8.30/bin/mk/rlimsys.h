/* $Id: rlimsys.h,v 5.0 1999/02/26 17:44:02 ksb Dist $
 * include file for rlimited system
 */

#if RESOURCE
extern int rlimsys();
extern char *rparse();
extern void rinit();
extern int r_fTrace;	/* use shell -cx for verbose trace		*/
#endif
