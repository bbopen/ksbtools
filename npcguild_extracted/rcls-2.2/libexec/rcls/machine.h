/* $Id: machine.h,v 2.0 2000/06/01 00:16:32 ksb Exp $
 * where one might set machine specifics (we have none)			(ksb)
 */

/* runtime loader dynamlic options
 */
#if !defined(RCLS_RTLD_OPTS)
#if defined(RTLD_NODELETE)
#define RCLS_RTLD_OPTS	(RTLD_NOW|RTLD_GLOBAL|RTLD_PARENT|RTLD_NODELETE)
#else
#define RCLS_RTLD_OPTS	(RTLD_NOW|RTLD_GLOBAL)
#endif
#endif
