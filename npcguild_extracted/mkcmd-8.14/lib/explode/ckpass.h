/*@Version $Revision: 6.0 $@*/
/*@Explode@*/
/* $Id: ckpass.h,v 6.0 2000/07/30 15:57:11 ksb Exp $
 * checks to see is the passwd (pcWord) matches the user's password
 * or (if not (char*)0) the "super pass" (pcSuperPass).
 */
extern int CheckPass(/* struct passwd *, pcSuperPass, pcWord */);
