/* $Id: machine.h,v 2.0 2000/05/30 12:30:00 ksb Exp $
 */

/* runtime loader dynamlic options
 */
#if !defined(RCDS_RTLD_OPTS)
#if defined(RTLD_NODELETE)
#define RCDS_RTLD_OPTS	(RTLD_NOW|RTLD_GLOBAL|RTLD_PARENT|RTLD_NODELETE)
#else
#define RCDS_RTLD_OPTS	(RTLD_NOW|RTLD_GLOBAL)
#endif
#endif
