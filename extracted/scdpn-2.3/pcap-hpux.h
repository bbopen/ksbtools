/* Some additional includes/defintiions needed for HP-UX 11.X
 * for some source files
 * $Id: pcap-hpux.h,v 2.2 2008/12/09 15:54:14 ksb Exp $
 */

#ifdef __hpux
#ifdef __ia64
/* From Itanium HP-UX 11.23's /usr/include/sys/mp.h: */
typedef union mpinfou {         /* For lint */
        struct pdk_mpinfo *pdkptr;
        struct mpinfo *pikptr;
} mpinfou_t;
#endif
#include <sys/mp.h>
#endif
