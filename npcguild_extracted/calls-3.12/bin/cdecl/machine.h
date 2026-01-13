/* $Id: machine.h,v 2.1 2000/06/17 22:42:19 ksb Exp $
 * from David Kaelbling <drk@sgi.com>
 */

#if !defined(IRIX)
#define IRIX   (defined(IRIX5)||defined(IRIX6))
#endif

#if !defined(USE_MALLOC_H)
#define USE_MALLOC_H   (defined(PTX)||defined(NETBSD)||defined(IBMR2)||defined(SUN5)||defined(MSDOS)||defined(IRIX5)||defined(IRIX6)||defined(ULTRIX))
#endif

#if !defined(NEED_MALLOC_EXTERN)
#define NEED_MALLOC_EXTERN     (!USE_MALLOC_H && !defined(NEXT2) && !defined(IRIX) && !defined(BSDI))
#endif

#if USE_MALLOC_H
#include <malloc.h>
#else
#if NEED_MALLOC_EXTERN
extern char *malloc(), *calloc();
#endif
#endif
