/* $Id: glob.h,v 2.1 1997/10/15 20:38:29 ksb Exp $
 * this macro could be in glob.h, or not...
 */
#define SamePath(Mglob, Mfile)	_SamePath(Mglob, Mfile, 1)
extern int _SamePath();
