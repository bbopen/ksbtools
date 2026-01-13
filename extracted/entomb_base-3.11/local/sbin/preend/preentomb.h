/* $Id: preentomb.h,v 3.1 2003/08/27 12:56:09 ksb Exp $
 * defaults for variables set from config file
 */

#define FULL_1		 90		/* % to start aging files	*/
#define FULL_2		 95		/* % to speed up aging		*/
#define FULL_3		100		/* % to age at max speed	*/
#define TIME_1		(24*60*60)	/* max time to live (one backup)*/
#define TIME_2		(6*60*60)	/* max time under stress	*/
#define MINTIME		(15*60)		/* min time in any case		*/
#define SECPERBYTE	(0.5)		/* min aging factor/byte in file*/

extern int PreenTomb();
