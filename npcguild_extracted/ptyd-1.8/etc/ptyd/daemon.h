/*
 * $Id: daemon.h,v 1.3 1997/11/29 17:58:50 ksb Exp $
 *
 * Joe daemon's init code
 */

#if HAVE_PROTO
extern void daemon(int (*)(), struct fd_set *, int);
#else
extern void daemon();
#endif
