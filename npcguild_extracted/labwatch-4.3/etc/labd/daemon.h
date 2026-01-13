/* $Id: daemon.h,v 3.2 1994/02/28 14:06:02 bj Exp $
 * Joe daemon's init code
 */

#if defined(__STDC__)
extern void daemon(SIGRET_T (*)(), struct fd_set *, int iDetatch);
#else
extern void daemon();
#endif
