/*@Header@*/
/*@Version: $Revision: 1.2 $@*/
/* $Id: unctime.h,v 1.2 2010/02/26 21:30:39 ksb Exp $
 * before us you need to include <sys/types.h> for the definition of (time_t)
 */

extern void unCtimeInit(void);
extern time_t unCtime(const char *pcWhen);
