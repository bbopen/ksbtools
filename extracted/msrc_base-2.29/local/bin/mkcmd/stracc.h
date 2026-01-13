/* $Id: stracc.h,v 8.0 1997/01/29 02:41:57 ksb Exp $
 * this code collects characters into a long dynamic buffer
 * the user may restart the "cursor", take the buffer, or add chars on the end
 */

void FirstChar();	/* provide first char and estimate buffer length */
void AddChar();		/* add a char and estimate buffer overflow	 */
char *TakeChars();	/* keep malloced buffer and (by var) length	 */
char *TakeLine();	/* same as TakeChars, but put on a \n too	 */
char *UseChars();	/* use malloced buffer and (by var) length	 */
