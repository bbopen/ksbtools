/*@Header@*/
/*@Version $Revision: 6.1 $@*/
/* $Id: ckpass.h,v 6.1 2010/02/26 21:30:39 ksb Exp $
 * checks to see is the passwd (pcWord) matches the user's password
 * or (if not (char*)0) the "super pass" (pcSuperPass).
 */
extern int CheckPass(/* struct passwd *, pcSuperPass, pcWord */);
