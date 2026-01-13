/* $Id: machine.h,v 2.1 2008/11/19 21:28:26 ksb Exp $
 */

/* Not all operating systems have the u_intN_t typedefs			(petef)
 */
#if !defined(NEED_SIZETYPES)
#if SUN5 || HPUX11
#define NEED_SIZETYPES 1
#endif
#endif

/* On Linux the pause(2) system call masks signals which is odd		(ksb)
 * because it is designed to wait for a signal.
 */

#if NEED_SIZETYPES
typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t;
typedef unsigned long u_int32_t;
#endif

/* To get the ethernet address on Solaris less than 10
 */
#if SUN5 && HOSTOS < 21000
extern struct ether_addr *ether_aton(char *s);
#endif

/* To make a FreeBSD look more like a SUN5
 */
#if defined(FREEBSD)
#define ether_addr_octet	octet
#endif
