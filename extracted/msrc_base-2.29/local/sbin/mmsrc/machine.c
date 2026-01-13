/* $Id: machine.c,v 1.3 2008/01/28 22:12:44 ksb Exp $
 * Bridge interface between autconf world and ksb's HOSTTYPE based code	(ksb)
 */
#include "config.h"
#include "machine.h"

#if NEED_MKDTEMP
#include "mkdtemp.c"
#endif

#if NEED_SETENV
#include "setenv.c"
#endif
