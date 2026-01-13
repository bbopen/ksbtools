/* $Id: daemon.h,v 3.2 1996/12/20 22:51:08 kb207252 Exp $
 * Joe daemon's init code
 */

#if defined(__STDC__)
extern void Daemon(void (*)(), struct fd_set *, int iDetatch);
#else
extern void Daemon();
#endif
